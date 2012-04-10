#include <stdio.h>
#include <stdlib.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t
#include <fcntl.h>
#include <unistd.h> // getcwd
#include <string.h>

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_name;
extern char *program_invocation_short_name;

#include "allocator/pool.h"
#include "lss-hcct.h"

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

// space saving global parameters - set by hcct_getenv()
UINT32          epsilon;
UINT32          phi;

// thread local storage
__thread unsigned          stack_idx;
__thread lss_hcct_node_t  *stack[STACK_MAX_DEPTH];
__thread lss_hcct_node_t  *hcct_root;
__thread pool_t           *node_pool;
__thread void             *free_list;
__thread UINT32            min, min_idx, num_queue_items, second_min_idx;
__thread lss_hcct_node_t **queue;
__thread int               queue_full;

#if BURSTING
extern unsigned long    sampling_interval;
extern unsigned long    burst_length;
extern unsigned short   burst_on;   // enable or disable bursting
extern __thread int     aligned;

// legal values from 0 up to shadow_stack_idx-1
extern __thread hcct_stack_node_t  shadow_stack[STACK_MAX_DEPTH];
extern __thread int                shadow_stack_idx; 
#endif


int hcct_getenv() {
    // initialize parameters
	char* value;
    
	value=getenv("EPSILON");
	if (value == NULL || value[0]=='\0')
	    epsilon=EPSILON;
    else {
        epsilon=strtoul(value, NULL, 0);
        if (epsilon==0) {
            epsilon=EPSILON;
            printf("[hcct] WARNING: invalid value specified for EPSILON, using default (%lu) instead\n", epsilon);
        }
    }
    
    value=getenv("PHI");
	if (value == NULL || value[0]=='\0')
	    phi=PHI;
    else {
        phi=strtoul(value, NULL, 0);
        if (phi==0) {
            phi=PHI;
            printf("[hcct] WARNING: invalid value specified for PHI, using default (%lu) instead\n", phi);            
        }
    }
    
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
        min=0xFFFFFFFF; // SENZA NON FUNZIONA: PERCHÉ????
        // IF: perche' il vecchio valore di min e' piu' piccolo
        // del minimo valore attualmente nell'array
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

    #else // Another version (faster? maybe slower!)

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
static void prune(lss_hcct_node_t* node) __attribute__((no_instrument_function)); // PERCHÉ QUA FA QUESTA PORCHERIA???
static void prune(lss_hcct_node_t* node) {

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

void hcct_enter(UINT32 routine_id, UINT16 call_site) {

    #if VERBOSE == 1
    printf("[lss_hcct] entering routine %lu\n", routine_id);
    #endif

    #if COMPARE_TO_CCT == 1 && BURSTING == 0
    rtn_enter_cct(routine_id, call_site);
    #endif

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

    #if VERBOSE == 1
    printf("[lss-hcct] exiting routine %lu\n", 
        stack[stack_idx]->routine_id);
    #endif    

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

    return 0;
}

#if BURSTING==1
void hcct_align() {        
    // reset CCT internal stack
    stack_idx=0;
    
    #if UPDATE_ALONG_TREE==1
    // Nota: come in burst.c (PLDI version) chiamo hcct_enter lungo tutto il ramo
    // Aspetto da controllare, perché sballa tutti i contatori lungo i rami
    // scan shadow stack from root to current routine (shadow_stack_idx-1)
    int i;
    for (i=0; i<shadow_stack_idx; ++i)
        hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site);
    aligned=1;
    #else
    // Uses hcct_enter only on actual routine and on nodes that have to be created along the path
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
    // Ottimizzazione (???) - scambio puntatori node e parent invece di accedere a cct_stack    
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
		fprintf(out, "v %lu %lu %lu %lu %hu\n", (unsigned long)root, (unsigned long)(root->parent),
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
	    // up to 10 for PID on 64 bits systems - however check /proc/sys/kernel/pid_max
	    char *dumpFileName=malloc(strlen(program_invocation_short_name)+16); // suffix: -PID.log\0
	    sprintf(dumpFileName, "%s-%d.log", program_invocation_short_name, tid);
        ds = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
        if (ds == -1) exit((printf("[hcct] ERROR: cannot create output file %s\n", dumpFileName), 1));
        out = fdopen(ds, "w");    

	    char* cwd=getcwd(NULL, 0);
	    	    	    
	    #if BURSTING
	    // c <tool> <epsilon> <phi> <sampling_interval> <burst_length>
	    fprintf(out, "c lss-hcct-burst %lu %lu %lu %lu\n", epsilon, phi, sampling_interval, burst_length);	    	    
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
	    fprintf(out, "p %lu\n", nodes); // for a sanity check in the analysis tool
	    fclose(out);
	    #endif
	    
	    #if DUMP_STATS==1
	    printf("[thread: %d] Total number of nodes: %lu\n", tid, nodes);		
        #endif
	#endif
}
