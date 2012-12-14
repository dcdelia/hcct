#include <stdio.h>
#include <stdlib.h>
#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t
#include <fcntl.h>
#include <unistd.h> // getcwd
#include <string.h>
#include <pthread.h>

#include "common.h"

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_name;
extern char *program_invocation_short_name;

pthread_key_t	tlsKey = 17;
int	tlsKey_active = 0;

typedef struct cct_node_s cct_node_t;
struct cct_node_s {
    ADDRINT     routine_id;    
    ADDRINT     call_site;
    UINT32      counter;
    cct_node_t* first_child;
    cct_node_t* next_sibling;    
};

typedef struct cct_tls_s cct_tls_t;
struct cct_tls_s {
	int cct_stack_idx;
	cct_node_t **cct_stack;
	cct_node_t *cct_root;
	UINT32      cct_nodes;
	char*		dumpPath;
};

cct_tls_t* __attribute__((no_instrument_function)) hcct_init()
{
	cct_tls_t* tls=malloc(sizeof(cct_tls_t));
	if (tls==NULL) {
		printf("[hcct] error while initializing thread local storage... Quitting!\n");
		exit(1);
	}
	
    tls->cct_stack_idx   = 0;
    tls->cct_nodes       = 1;   
             
    tls->cct_stack=calloc(STACK_MAX_DEPTH, sizeof(cct_node_t));         
	if (tls->cct_stack == NULL) {
			printf("[hcct] error while initializing cct internal stack... Quitting!\n");
			exit(1);
	}
							
    tls->cct_stack[0]=(cct_node_t*)malloc(sizeof(cct_node_t));    
    if (tls->cct_stack[0] == NULL) {
			printf("[hcct] error while initializing cct root node... Quitting!\n");
			exit(1);
	}

    tls->cct_stack[0]->first_child = NULL;
    tls->cct_stack[0]->next_sibling = NULL;
    tls->cct_stack[0]->routine_id = 0;
    tls->cct_stack[0]->call_site = 0;
    tls->cct_stack[0]->counter = 1;
    tls->cct_root = tls->cct_stack[0];
    
    tls->dumpPath=getenv("DUMPPATH");
	if (tls->dumpPath == NULL || tls->dumpPath[0]=='\0')
	    tls->dumpPath=NULL;

    return tls;
}

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

#if DUMP_TREE==1
void __attribute__ ((no_instrument_function)) hcct_dump_map() {
	
	pid_t pid=syscall(__NR_getpid);
		
	// Command to be executed: "cp /proc/<PID>/maps <name>.map\0" => 9+10+6+variable+5 bytes needed
	char command[BUFLEN+1];
	sprintf(command, "cp -f /proc/%d/maps %s.map && chmod +w %s.map", pid, program_invocation_short_name, program_invocation_short_name);
	
	int ret=system(command);
	if (ret!=0) printf("[profiler] WARNING: unable to read currently mapped memory regions from /proc\n");	

}
#endif

void __attribute__((no_instrument_function)) hcct_dump(cct_tls_t* tls)
{
	#if DUMP_TREE==1
	unsigned long nodes=0;
	UINT64 cct_enter_events=0;
	FILE* out;
	pid_t tid=syscall(__NR_gettid);	    
	
    int ds;	    
	char dumpFileName[BUFLEN+1];	    
	if (tls->dumpPath==NULL) {
	    sprintf(dumpFileName, "%s-%d.tree", program_invocation_short_name, tid);
    } else {            
	    sprintf(dumpFileName, "%s/%s-%d.tree", tls->dumpPath, program_invocation_short_name, tid);
    }
    ds = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
    if (ds == -1) {
		printf("[hcct] ERROR: cannot create output file %s\n", dumpFileName);
		exit(1);
	}
    out = fdopen(ds, "w");    	    	    
	
	// c <tool>
	fprintf(out, "c cct \n"); // do not remove the white space between cct and \n :)	    	    	
	    
	// c <command> <process/thread id> <working directory>
	char* cwd=getcwd(NULL, 0);
	fprintf(out, "c %s %d %s\n", program_invocation_name, tid, cwd);	    
	free(cwd);	   
	
	hcct_dump_aux(out, tls->cct_root, &nodes, &cct_enter_events, NULL);
	cct_enter_events--; // root node is a dummy node with counter 1
		
	// p <nodes> <enter_events>
	fprintf(out, "p %lu %llu\n", nodes, cct_enter_events); // #nodes used for a sanity check in the analysis tool
	fclose(out);	
	
	#endif
	
}

void __attribute__((no_instrument_function)) tlsDestructor(void* value) {
	cct_tls_t* tls=(cct_tls_t*) value;
	
	hcct_dump(tls);
		
	free_cct(tls->cct_root); // recursively free the CCT	
	
	free(tls->cct_stack);
	free(tls->dumpPath);
	free(tls);
	
	pthread_setspecific(tlsKey, NULL);
	
	#if SHOW_MESSAGES==1
	printf("[hcct] dump completed for thread %ld\n", syscall(__NR_gettid));
	#endif
}

void __attribute__((no_instrument_function)) hcct_enter(ADDRINT routine_id, ADDRINT call_site)
{			
	cct_tls_t* tls=(cct_tls_t*)pthread_getspecific(tlsKey);
	if (tls==NULL) {
		#if SHOW_MESSAGES==1
		printf("[hcct] TLS non initialized yet for thread %ld\n", syscall(__NR_gettid));
		#endif
		if (tlsKey_active==0 && pthread_key_create(&tlsKey, tlsDestructor)) {
			printf("[profiler] error while creating key for TLS - exiting...\n");
            exit(1);   
		} else tlsKey_active=1;
		pthread_setspecific(tlsKey, hcct_init());
		tls=(cct_tls_t*)pthread_getspecific(tlsKey);
		if (tls==NULL) {
			printf("[hcct] WARNING: unable to create TLS... skipping this rtn_enter event\n");
			return;
		}
	} // else: TLS already initialized 
		
	cct_node_t *parent=tls->cct_stack[tls->cct_stack_idx++];	 			
    cct_node_t *node;
    
    for (node=parent->first_child; node!=NULL; node=node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;   

    if (node!=NULL) {
        tls->cct_stack[tls->cct_stack_idx]=node;       
        node->counter++;        
        return;
    }

    tls->cct_nodes++;    
    node=(cct_node_t*)malloc(sizeof(cct_node_t)); // TODO check ritorno malloc??
    if (node==NULL) {
		printf("[hcct] error while allocating new CCT node... Quitting!\n");
		exit(1);
	}
    
    tls->cct_stack[tls->cct_stack_idx] = node;
    node->routine_id = routine_id;    
    node->first_child = NULL;    
    node->counter = 1;    
    node->call_site =  call_site;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}

void __attribute__((no_instrument_function)) hcct_exit()
{
    
    cct_tls_t* tls=(cct_tls_t*)pthread_getspecific(tlsKey);
	if (tls==NULL) {
		printf("[hcct] WARNING: tree created after an exit event!\n");
		pthread_setspecific(tlsKey, hcct_init());
		tls=(cct_tls_t*)pthread_getspecific(tlsKey);
		if (tls==NULL) {			
			printf("[hcct] WARNING: unable to create TLS... skipping this rtn_exit event\n");
			return;
		}
	} else {
		tls->cct_stack_idx--;
	}
    
}

// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program start - tid %d\n", tid);
        #endif
        
        if (tlsKey_active==0 && pthread_key_create(&tlsKey, tlsDestructor)) {
			printf("[profiler] error while creating key for TLS - exiting...\n");
            exit(1);   
		} else tlsKey_active=1;
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		void *tls=(void*)pthread_getspecific(tlsKey);
		if (tls==NULL)
			printf("[hcct] WARNING: unable to access TLS for main thread - %ld", syscall(__NR_gettid));
		else 
			tlsDestructor(tls);		
		
		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program exit - tid %d\n", tid);
        #endif
        
        #if DUMP_TREE==1
		hcct_dump_map();
		#endif
}

// Routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{		
		hcct_enter((unsigned long)this_fn, (unsigned long)call_site);
}

// Routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
        hcct_exit();
}

// wrap pthread_exit()
void __attribute__((no_instrument_function)) __wrap_pthread_exit(void *value_ptr)
{
		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
		printf("[profiler] pthread_exit - tid %d\n", tid);
		#endif
		        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                                
        
        // Retrieve original routine address and argument        
        void* orig_arg=((void**)arg)[1];        
        void *(*start_routine)(void*)=((void**)arg)[0];
        free(arg);
        
		// Run actual application thread's init routine
        void* ret=(*start_routine)(orig_arg);

		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
        printf("[profiler] return - tid %d\n", tid);
        #endif
        
        return ret;
}

// wrap pthread_create
int __attribute__((no_instrument_function)) __wrap_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
        #if SHOW_MESSAGES==1
        pid_t tid=syscall(__NR_gettid);
        printf("[profiler] spawning a new thread from tid %d\n", tid);
        #endif
        
        void** params=malloc(2*sizeof(void*));
        if (params==NULL) {
			printf("[profiler] error while spawning new thread from tid %ld\n", syscall(__NR_gettid));
			exit(1);
		}
        params[0]=start_routine;
        params[1]=arg;        
        return __real_pthread_create(thread, attr, aux_pthread_create, params);
}
