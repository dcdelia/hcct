#include <stdio.h>
#include <stdlib.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t
#include <fcntl.h>
#include <unistd.h> // getcwd
#include <string.h>
#include <math.h> // ceil

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_name;
extern char *program_invocation_short_name;

#include "lss-hcct.h"
#include "pool.h"

// swap macro
#define swap_and_decr(a,b) {            \
    lss_hcct_node_t *tmp = queue[a];    \
    queue[a] = queue[b];                \
    queue[b--] = tmp;                   \
}

// monitoring macros
#define IsMonitored(x)      ((x)->additional_info==1)
#define SetMonitored(x)     ((x)->additional_info=1)
#define UnsetMonitored(x)   ((x)->additional_info=0)

// global parameters set by hcct_getenv()
UINT32  epsilon;
UINT32  phi;
char*   dumpPath;

// thread local storage
__thread unsigned			stack_idx;
__thread lss_hcct_node_t	*stack[STACK_MAX_DEPTH];
__thread lss_hcct_node_t	*hcct_root;
__thread pool_t				*node_pool;
__thread void				*free_list;
__thread UINT32				min, min_idx, num_queue_items, second_min_idx;
__thread lss_hcct_node_t	**queue;
__thread int				queue_full;
__thread UINT64				lss_enter_events;

#if BURSTING
extern unsigned long    sampling_interval;
extern unsigned long    burst_length;
extern unsigned short   burst_on;   // enable or disable bursting
extern __thread int     aligned;
extern __thread UINT64	burst_enter_events;

// legal values go from 0 to shadow_stack_idx-1
extern __thread hcct_stack_node_t  shadow_stack[STACK_MAX_DEPTH];
extern __thread int                shadow_stack_idx; 
#endif


// get parameters from environment variables
int hcct_getenv() {
	char *value;
	
	char *end; // strtod()
	double d;
    
	value=getenv("EPSILON");
	if (value == NULL || value[0]=='\0')
	    epsilon=EPSILON;
    else {
    /* // Old format: integer value specified as 1/epsilon
        epsilon=strtoul(value, NULL, 0);
        if (epsilon==0) {
            epsilon=EPSILON;
            printf("[hcct] WARNING: invalid value specified for EPSILON, using default (%lu) instead\n", epsilon);
        }
    */
		d=strtod(value, &end);
		if (d<=0 || d>1.0) { // 0 is an error code
            epsilon=EPSILON;
            printf("[hcct] WARNING: invalid value specified for EPSILON, using default (%f) instead\n", 1.0/epsilon);
        } else {
			epsilon=(UINT32)ceil(1.0/d);
		}	
    }
    
    value=getenv("PHI");
	if (value == NULL || value[0]=='\0')
	    phi=PHI;
    else {
        /* //Old format
        phi=strtoul(value, NULL, 0);
        if (phi==0) {
            phi=PHI;
            printf("[hcct] WARNING: invalid value specified for PHI, using default (%lu) instead\n", phi);            
        }
        */
        d=strtod(value, &end);
		if (d<=0 || d >= 1.0) { // 0 is an error code
            phi=epsilon/10;
            printf("[hcct] WARNING: invalid value specified for PHI, using default (PHI=10*EPSILON) instead\n");
        } else {
			phi=(UINT32)ceil(1.0/d);
			if (phi>=EPSILON) {
				phi=epsilon/10;
				printf("[hcct] WARNING: PHI must be greater than EPSILON, using PHI=10*EPSILON instead");
			}
		}	        
    }
    
    dumpPath=getenv("DUMPPATH");
	if (dumpPath == NULL || dumpPath[0]=='\0')
	    dumpPath=NULL;
    
    return 0;
}

lss_hcct_node_t* hcct_get_root() {
    return hcct_root;
}

#if INLINE_UPD_MIN==0
void update_min() __attribute__((no_instrument_function))
{

    #if 0 // O(n) - classic minimum search
    int i;
    min=0xFFFFFFFF;
    for (i=0; i<num_queue_items; ++i)
        if (queue[i]->counter < min) {
            min=queue[i]->counter;
            min_idx=i;
        }
    #else

    #if 1 // See paper for details
    int i=min_idx+1; // Obviously, min has changed!
    
    for (; i < num_queue_items && queue[i]->counter != min; ++i);
        
    if (i>=num_queue_items) {
        min=0xFFFFFFFF; // actual min in the array is bigger than old min
        for (i=0; i<num_queue_items; ++i) {
            if (queue[i]->counter < min) {
                min=queue[i]->counter;
                min_idx=i;
            }
        }
    } else {
        min=queue[i]->counter;
        min_idx=i;
    }

    #else // Another version (faster, slower?)

    int i=min_idx+1, j=0;
    UINT32 tmp=queue[min_idx]->counter;
    
    for (; i < num_queue_items && queue[i]->counter !=min; ++i) {
        if (queue[i]->counter < tmp) { // keeping track of minimum
            j=i;
            tmp=queue[i]->counter;
        }        
    }
    
    if (i>=num_queue_items) {
        min=queue[min_idx]->counter;
        for (i=0; i<min_idx; ++i) {
            if (queue[i]->counter < min) {
                min=queue[i]->counter;
                min_idx=i;
            }
        }
        if (tmp<min) {
            min=tmp;
            min_idx=j;
        }
    } else { 
        min=queue[i]->counter;
        min_idx=i;
    }
    #endif
    #endif
}
#endif

// to be used only on an unmonitored leaf!
static void __attribute__((no_instrument_function)) prune(lss_hcct_node_t* node) {

    lss_hcct_node_t *p, *c;

    if (node==hcct_root) return;
    p=node->parent;

    if (p->first_child==node) {
        p->first_child=node->next_sibling;
        if (p->first_child==NULL && IsMonitored(p)==0)
            prune(p); // cold node becomes a leaf: can be pruned!        
    } 
    else for (c=p->first_child; c!=NULL; c=c->next_sibling)
            if (c->next_sibling==node) {
                c->next_sibling=node->next_sibling;
                break;
            }

    pool_free(node, free_list);
}

void hcct_enter(UINT32 routine_id, UINT32 call_site) {

	++lss_enter_events;
	
    lss_hcct_node_t *parent = stack[stack_idx];
    lss_hcct_node_t *node;    

    // check if calling context is already in the tree
    for (node = parent->first_child; 
         node != NULL; 
         node = node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;
    
    // node was already in the tree
    if (node != NULL) {

        stack[++stack_idx] = node;

        if (IsMonitored(node)) {
            node->counter++;
            return;
        }
    } 

    // add new child to parent
    else {
        pool_alloc(node_pool, free_list, 
                   node, lss_hcct_node_t);
        node->routine_id    = routine_id;
        node->call_site     = call_site;
        node->first_child   = NULL;
        node->next_sibling  = parent->first_child;
        node->parent        = parent;
        parent->first_child = node;
        stack[++stack_idx]  = node;
    }

    // Mark node as monitored
    SetMonitored(node);

    // if table is not full
    if (!queue_full) {
        #if KEEP_EPS
        node->eps = 0;
        #endif
        node->counter = 1;
        queue[num_queue_items++] = node;
        queue_full = num_queue_items == epsilon;
        return;
    }

    #if INLINE_UPD_MIN == 1
 
    // version with swaps
    #if 0
    while (++min_idx < epsilon)
        if (queue[min_idx]->counter == min) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    min_idx = epsilon-1;
    swap_and_decr(second_min_idx, min_idx);
    for (i = 0; i <= min_idx; ++i) {
        if (queue[i]->counter > min) continue;
        if (queue[i]->counter < min)  {
            second_min_idx = i;
            min_idx = epsilon-1;
            min = queue[i]->counter;
        }
        swap_and_decr(i, min_idx);
    }
    end: 
    #endif

    #if UPDATE_MIN_SENTINEL == 1

    // version with second_min + sentinel
    while (queue[++min_idx]->counter > min);

    if (min_idx < epsilon) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    for (min_idx = 0, i = 0; i < epsilon; ++i) {
        if (queue[i]->counter < min) {
            second_min_idx = min_idx;
            min_idx = i;
            min = queue[i]->counter;
        }
    }
    queue[epsilon]->counter = min;
    end: 

    // version with second_min
    #else
    while (++min_idx < epsilon)
        if (queue[min_idx]->counter == min) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    for (min_idx = 0, i = 0; i < epsilon; ++i) {
        if (queue[i]->counter < min) {
            second_min_idx = min_idx;
            min_idx = i;
            min = queue[i]->counter;
        }
    }
    end: 
    #endif
    
    #else
    // no explicit inlining: -O3 should do it anyway, though
    update_min();
    #endif

    #if KEEP_EPS
    node->eps=min;
    #endif

    node->counter = min + 1;

    UnsetMonitored(queue[min_idx]);

    if (queue[min_idx]->first_child==NULL)
        prune(queue[min_idx]);

    queue[min_idx]=node;
}

void hcct_exit()
{
    stack_idx--;
}

int hcct_init()
{
    // initialize custom memory allocator
    node_pool = 
        pool_init(PAGE_SIZE, sizeof(lss_hcct_node_t), &free_list);
    if (node_pool == NULL) {
			printf("[hcct] error while initializing allocator... Quitting!\n");
            return -1;
	}
    
    // create dummy root node
    pool_alloc(node_pool, free_list, hcct_root, lss_hcct_node_t);
    if (hcct_root == NULL) {
			printf("[hcct] error while initializing hcct root node... Quitting!\n");
			return -1;
	}
    
    hcct_root->first_child  = NULL;
    hcct_root->next_sibling = NULL;
    hcct_root->counter      = 1;
    hcct_root->routine_id   = 0;
    hcct_root->call_site	= 0;
    hcct_root->parent       = NULL;
    SetMonitored(hcct_root);

    // initialize stack
    stack[0]  = hcct_root;
    stack_idx = 0;

    // create lazy priority queue
    #if UPDATE_MIN_SENTINEL == 1
    queue = (lss_hcct_node_t**)malloc((epsilon+1)*sizeof(lss_hcct_node_t*));
    pool_alloc(node_pool, free_list, queue[epsilon], lss_hcct_node_t);
    if (queue[epsilon] == NULL) {
			printf("[hcct] error while initializing lazy priority queue... Quitting!\n");
			return -1;
	}
    queue[epsilon]->counter = min = 0;
    #else
    queue = (lss_hcct_node_t**)malloc(epsilon*sizeof(lss_hcct_node_t*));
    #endif
    if (queue == NULL) {
			printf("[hcct] error while initializing lazy priority queue... Quitting!\n");
			return -1;
	}
    
    queue[0]        = hcct_root;
    num_queue_items = 1; // goes from 0 to epsilon
    queue_full      = 0;
    min_idx         = epsilon-1;
    second_min_idx  = 0;
    
    lss_enter_events=0;

    return 0;
}

#if BURSTING==1
void hcct_align() {        
    // reset CCT internal stack
    stack_idx=0;
    
    #if UPDATE_ALONG_TREE==1
    // Updates all nodes related to routines actually in the the stack
    int i;
    for (i=0; i<shadow_stack_idx; ++i)
        hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site);
    aligned=1;
    #else
    // Uses hcct_enter only on current routine and on nodes that have to be created along the path
    lss_hcct_node_t *parent, *node;
    int i;
    
    // scan shadow stack
    #if 0
    for (i=0; i<shadow_stack_idx-1;) {
        parent=stack[stack_idx++];    
        for (node=parent->first_child; node!=NULL; node=node->next_sibling)
            if (node->routine_id == shadow_stack[i].routine_id &&
                node->call_site == shadow_stack[i].call_site) break;
        if (node!=NULL) {
            // No need to update counter here
            stack[stack_idx]=node;
            ++i;            
        } else {
            // I have to create additional nodes in the next for cycle
            --stack_idx;
            break;            
        }        
    }
    #else
    // Should be better :)
    parent=stack[stack_idx];
    for (i=0; i<shadow_stack_idx-1;) {
        for (node=parent->first_child; node!=NULL; node=node->next_sibling)
            if (node->routine_id == shadow_stack[i].routine_id &&
                node->call_site == shadow_stack[i].call_site) break;
        if (node!=NULL) {
            // No need to update counter here
            stack[++stack_idx]=node;
            parent=node;
            ++i;            
        } else {
            // I have to create additional nodes in the next for cycle            
            break;            
        }        
    }    
    #endif
    
    // update counters only for current routine and for new nodes created along the path
    for (; i<shadow_stack_idx; ++i)
        hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site);
    
    aligned=1;
    #endif
}
#endif


void __attribute__((no_instrument_function)) hcct_dump_aux(FILE* out, lss_hcct_node_t* root, unsigned long *nodes)
{
        if (root==NULL) return;
        
        (*nodes)++;
        lss_hcct_node_t* ptr;

		#if DUMP_TREE==1
		// Syntax: v <node id> <parent id> <counter> <routine_id> <call_site>
		// Addresses in hexadecimal notation (useful for addr2line)
		fprintf(out, "v %lx %lx %lu %lx %lx\n", (unsigned long)root, (unsigned long)(root->parent),
		                                        root->counter, root->routine_id, root->call_site);
        #endif
        
        for (ptr = root->first_child; ptr!=NULL; ptr=ptr->next_sibling)
                hcct_dump_aux(out, ptr, nodes);
}

void hcct_dump()
{
	#if DUMP_STATS==1 || DUMP_TREE==1
	unsigned long nodes=0;
	FILE* out;
	pid_t tid=syscall(__NR_gettid);	    
	
	    #if DUMP_TREE==1
	    int ds;
	    // up to 10 digits for PID on 64 bits systems - however check /proc/sys/kernel/pid_max
	    // TODO FIX %d 10 digits
	    char *dumpFileName;	    
	    if (dumpPath==NULL) {
	        dumpFileName=malloc(strlen(program_invocation_short_name)+17); // suffix: -PID.tree\0
	        sprintf(dumpFileName, "%s-%d.tree", program_invocation_short_name, tid);
        } else {
            dumpFileName=malloc(strlen(dumpPath)+1+strlen(program_invocation_short_name)+17); // suffix: -PID.tree\0
	        sprintf(dumpFileName, "%s/%s-%d.tree", dumpPath, program_invocation_short_name, tid);
        }
        ds = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
        if (ds == -1) exit((printf("[hcct] ERROR: cannot create output file %s\n", dumpFileName), 1));
        out = fdopen(ds, "w");    

	    char* cwd=getcwd(NULL, 0);
	    	    	    
	    #if BURSTING
	    // c <tool> <epsilon> <phi> <sampling_interval> <burst_length> <burst_enter_events>
	    fprintf(out, "c lss-hcct-burst %lu %lu %lu %lu %llu\n", epsilon, phi, sampling_interval, burst_length, burst_enter_events);	    	    
	    #else
	    // c <tool> <epsilon> <phi>
	    fprintf(out, "c lss-hcct %lu %lu\n", epsilon, phi);	    	    
	    #endif
	    
	    // c <command> <process/thread id> <working directory>
	    fprintf(out, "c %s %d %s\n", program_invocation_name, tid, cwd);
	    
	    free(dumpFileName);
	    free(cwd);
	    #endif
	
	hcct_dump_aux(out, hcct_get_root(), &nodes);
	
	    #if DUMP_TREE==1
	    // p <nodes> <enter_events>
	    fprintf(out, "p %lu %llu\n", nodes, lss_enter_events); // #nodes used for a sanity check in the analysis tool
	    fclose(out);
	    #endif
	    
	    #if DUMP_STATS==1
	    printf("[thread: %d] Total number of nodes: %lu\n", tid, nodes);
	    printf("[thread: %d] Total number of routine enter events: %llu\n", tid, lss_enter_events);
        #endif
	#endif
}
