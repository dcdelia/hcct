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

#include "cct.h"

#if USE_MALLOC==0
	#include "pool.h"
	#define PAGE_SIZE		1024
	__thread pool_t     *cct_pool;
	__thread void       *cct_free_list;
#endif

// thread local storage
__thread int         cct_stack_idx;
__thread cct_node_t *cct_stack[STACK_MAX_DEPTH];
__thread cct_node_t *cct_root;
__thread UINT32      cct_nodes;

// global parameters set by hcct_getenv()
char*   dumpPath;

#if BURSTING==1
extern unsigned long    sampling_interval;
extern unsigned long    burst_length;
extern unsigned short   burst_on;   // enable or disable bursting
extern __thread int     aligned;
extern __thread UINT64	exhaustive_enter_events;

// legal values go from 0 to shadow_stack_idx-1
extern __thread hcct_stack_node_t  shadow_stack[STACK_MAX_DEPTH];
extern __thread int                shadow_stack_idx; 
#elif PROFILE_TIME==1
extern UINT32						sampling_interval;
extern __thread UINT64				thread_tics;

// legal values go from 0 to shadow_stack_idx-1
extern __thread hcct_stack_node_t	shadow_stack[STACK_MAX_DEPTH];
extern __thread int					shadow_stack_idx; 
#endif

// get parameters from environment variables
static void __attribute__((no_instrument_function)) hcct_getenv() {
	dumpPath=getenv("DUMPPATH");
	if (dumpPath == NULL || dumpPath[0]=='\0')
	    dumpPath=NULL;	
}

cct_node_t* hcct_get_root()
{
    return cct_root;
}

void hcct_init()
{
	// hcct_init might have been invoked already once before trace_begin() is executed
    if (cct_root!=NULL) return;
    
    hcct_getenv(); // TODO
    
    cct_stack_idx   = 0;
    cct_nodes       = 1;
    
    #if SHOW_MESSAGES==1
    pid_t tid=syscall(__NR_gettid);
    printf("[cct] initializing data structures for thread %d\n", tid);
    #endif
                          
    #if USE_MALLOC==1        
    cct_stack[0]=(cct_node_t*)malloc(sizeof(cct_node_t));    
    if (cct_stack[0] == NULL) {
			printf("[cct] error while initializing cct root node... Quitting!\n");
			exit(1);
	}    		
    #else
    // initialize custom memory allocator    
    cct_pool = pool_init(PAGE_SIZE, sizeof(cct_node_t), &cct_free_list);
    if (cct_pool == NULL) {
			printf("[cct] error while initializing allocator... Quitting!\n");
			exit(1);
	}

    pool_alloc(cct_pool, cct_free_list, cct_stack[0], cct_node_t);
    if (cct_stack[0] == NULL) {
			printf("[cct] error while initializing cct root node... Quitting!\n");
			exit(1);
	}
	#endif

    cct_stack[0]->first_child = NULL;
    cct_stack[0]->next_sibling = NULL;
    cct_stack[0]->routine_id = 0;
    cct_stack[0]->call_site = 0;
    cct_stack[0]->counter = 1;
    cct_root = cct_stack[0];
}

#if BURSTING==1
void hcct_align() {        
    // reset CCT internal stack
    cct_stack_idx=0;
    
    #if UPDATE_ALONG_TREE==1
    // Updates all nodes related to routines actually in the the stack
    int i;
    for (i=0; i<shadow_stack_idx; ++i)
        hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site);
    aligned=1;
    #else
    // Uses hcct_enter only on current routine and on nodes that have to be created along the path
    cct_node_t *parent, *node;
    int i;
    
    // scan shadow stack
    #if 0
    for (i=0; i<shadow_stack_idx-1;) {
        parent=cct_stack[cct_stack_idx++];    
        for (node=parent->first_child; node!=NULL; node=node->next_sibling)
            if (node->routine_id == shadow_stack[i].routine_id &&
                node->call_site == shadow_stack[i].call_site) break;
        if (node!=NULL) {
            // No need to update counter here
            cct_stack[cct_stack_idx]=node;
            ++i;            
        } else {
            // I have to create additional nodes in the next for cycle
            --cct_stack_idx;
            break;            
        }        
    }
    #else
    // Should be better :)
    parent=cct_stack[cct_stack_idx];
    for (i=0; i<shadow_stack_idx-1;) {
        for (node=parent->first_child; node!=NULL; node=node->next_sibling)
            if (node->routine_id == shadow_stack[i].routine_id &&
                node->call_site == shadow_stack[i].call_site) break;
        if (node!=NULL) {
            // No need to update counter here
            cct_stack[++cct_stack_idx]=node;
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
#elif PROFILE_TIME==1
void hcct_align(UINT32 increment) {
	// reset CCT internal stack
    cct_stack_idx=0;
	
	// walk the shadow stack
	int i;
    for (i=0; i<shadow_stack_idx-1; ++i)
        hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site, 0);
    hcct_enter(shadow_stack[i].routine_id, shadow_stack[i].call_site, increment);
	
}
#endif

#if PROFILE_TIME==0
void hcct_enter(ADDRINT routine_id, ADDRINT call_site)
#else
void hcct_enter(ADDRINT routine_id, ADDRINT call_site, UINT32 increment)
#endif
{				
	cct_node_t *parent=cct_stack[cct_stack_idx++]; 				
	if (parent==NULL) { // for instance, after trace_end() some events may still occur!		
		#if SHOW_MESSAGES==1		
		printf("[cct] rtn_enter event while CCT not initialized yet for thread %ld\n", syscall(__NR_gettid));
		#endif
		
		hcct_init();		

		parent=cct_stack[cct_stack_idx];
		if (parent==NULL) {
			printf("[hcct] unable to initialize HCCT... Quitting!\n");
			exit(1);
		}
	}
	
    cct_node_t *node;    
    for (node=parent->first_child; node!=NULL; node=node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;   

    if (node!=NULL) {
        cct_stack[cct_stack_idx]=node;
        #if PROFILE_TIME==0
        node->counter++;
        #else
        node->counter+=increment;
        #endif
        return;
    }

    ++cct_nodes;
    #if USE_MALLOC==1
    node=(cct_node_t*)malloc(sizeof(cct_node_t));
    #else    
    pool_alloc(cct_pool, cct_free_list, node, cct_node_t);
    #endif
    cct_stack[cct_stack_idx] = node;
    node->routine_id = routine_id;    
    node->call_site =  call_site;    
    #if PROFILE_TIME==0
    node->counter = 1;
    #else
    node->counter=increment;
    #endif
    node->first_child = NULL;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}

void hcct_exit()
{
    cct_stack_idx--;
}

#if DUMP_TREE==1
static void __attribute__((no_instrument_function)) hcct_dump_aux(FILE* out,
        cct_node_t* root, unsigned long *nodes, UINT64* cct_enter_events, cct_node_t* parent)
{
	if (root==NULL) return;
        
	(*nodes)++;
	(*cct_enter_events)+=root->counter;
	cct_node_t* ptr;

	// Syntax: v <node id> <parent id> <counter> <routine_id> <call_site>
	// Addresses in hexadecimal notation (useful for addr2line)
	fprintf(out, "v %lx %lx %lu %lx %lx\n", (unsigned long)root, (unsigned long)(parent),
	                                        root->counter, root->routine_id, root->call_site);
      
	for (ptr = root->first_child; ptr!=NULL; ptr=ptr->next_sibling)
		hcct_dump_aux(out, ptr, nodes, cct_enter_events, root);		
}
#endif

#if USE_MALLOC==1
static void __attribute__((no_instrument_function)) free_cct(cct_node_t* node) {
	// free(NULL) is ok :)   
   cct_node_t *ptr, *tmp;
    for (ptr=node->first_child; ptr!=NULL;) {
		tmp=ptr->next_sibling;
        free_cct(ptr);
        ptr=tmp;
    }    
    free(node);
}
#endif

void hcct_dump()
{
	pid_t tid;
	
	#if SHOW_MESSAGES==1
    tid=syscall(__NR_gettid);
    printf("[cct] removing data structures for thread %d\n", tid);
    #endif
	
	#if DUMP_TREE==1
	unsigned long nodes=0;
	UINT64 cct_enter_events=0;
	FILE* out;
	tid=syscall(__NR_gettid);	    
	
    int ds;	    
	char dumpFileName[BUFLEN+1];	    
	if (dumpPath==NULL) {
	    sprintf(dumpFileName, "%s-%d.tree", program_invocation_short_name, tid);
    } else {            
	    sprintf(dumpFileName, "%s/%s-%d.tree", dumpPath, program_invocation_short_name, tid);
    }
    ds = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
    if (ds == -1) exit((printf("[cct] ERROR: cannot create output file %s\n", dumpFileName), 1));
    out = fdopen(ds, "w");    
	    	    
	#if BURSTING==1
	// c <tool> <sampling_interval> <burst_length> <exhaustive_enter_events>    
	fprintf(out, "c cct-burst %lu %lu %llu\n", sampling_interval, burst_length, exhaustive_enter_events);	    
	#elif PROFILE_TIME==1
	// c <tool> <sampling_interval> <thread_tics>
	fprintf(out, "c cct-time %lu %llu\n", sampling_interval, thread_tics);
	#else
	// c <tool>
	fprintf(out, "c cct \n"); // do not remove the white space between cct and \n :)	    	    
	#endif
	    
	// c <command> <process/thread id> <working directory>
	char* cwd=getcwd(NULL, 0);
	fprintf(out, "c %s %d %s\n", program_invocation_name, tid, cwd);	    
	free(cwd);	   
	
	hcct_dump_aux(out, hcct_get_root(), &nodes, &cct_enter_events, NULL);
	cct_enter_events--; // root node is a dummy node with counter 1
		
	// p <nodes> <enter_events>
	fprintf(out, "p %lu %llu\n", nodes, cct_enter_events); // #nodes used for a sanity check in the analysis tool
	fclose(out);
	#endif
	    	    	
	#if USE_MALLOC==1
	// Recursively free the CCT
	free_cct(cct_root);	
	#else
	// Free memory used by custom allocator
	pool_cleanup(cct_pool);	
	#endif
	
	// for instance, after trace_end() some events may still occur!	
	cct_root=NULL; // see first instruction in hcct_init()
	cct_stack_idx=0; cct_stack[0]=NULL; // see if parent==NULL in hcct_enter()
		
	#if SHOW_MESSAGES==1
	printf("[cct] dump completed for thread %ld\n", syscall(__NR_gettid));
	#endif
	
}
