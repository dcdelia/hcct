#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_short_name;

#define BURSTING 1
#include "config.h"

// timer globals
pthread_t	timerThreadID;
UINT16		burst_on;
UINT32		sampling_interval;
UINT32		burst_length;

// TLS
__thread UINT64				exhaustive_enter_events;
__thread hcct_stack_node_t	shadow_stack[STACK_MAX_DEPTH];
__thread UINT16				shadow_stack_idx; // legal values: 0 to shadow_stack_idx-1
__thread UINT16				aligned;

#ifdef PROFILER_CCT
#include "cct.h"
#else
#ifdef PROFILER_EMPTY
#include "empty.h"
#else
#include "lss-hcct.h"
#endif
#endif

#if DUMP_TREE==1 && PROFILER_EMPTY==0
static void __attribute__ ((no_instrument_function)) hcct_dump_map() {
	
	pid_t pid=syscall(__NR_getpid);
				
	char command[BUFLEN+1];
	sprintf(command, "cp -f /proc/%d/maps %s.map && chmod +w %s.map", pid, program_invocation_short_name, program_invocation_short_name);
	
	int ret=system(command);
	if (ret!=0) printf("[profiler] WARNING: unable to read currently mapped memory regions from /proc\n");	
	
}
#endif

// timer thread
void __attribute__((no_instrument_function)) timerThread(void* arg)
{
    struct timespec enabled, disabled;
    enabled.tv_sec=0;
    enabled.tv_nsec=burst_length;
    disabled.tv_sec=0;
    disabled.tv_nsec=sampling_interval-burst_length;
    
    while(1) {
        if (burst_on!=0) { 
            // disable bursting
            burst_on=0;
            clock_nanosleep(TIMER_TYPE, 0, &disabled, NULL);
        } else {
            // enable bursting
            burst_on=1;
            clock_nanosleep(TIMER_TYPE, 0, &enabled, NULL);
        }
    }
}

// initialize timer thread
void __attribute__ ((no_instrument_function)) init_bursting()
{
        burst_on=1;
                
        char* value;
        value=getenv("SINTVL");
        if (value == NULL || value[0]=='\0')
            sampling_interval=SAMPLING_INTERVAL;
        else {
            sampling_interval=strtoul(value, NULL, 0);
            if (sampling_interval==0) {
                sampling_interval=SAMPLING_INTERVAL;
                printf("[profiler] WARNING: invalid value specified for SINTVL, using default (%lu) instead\n", sampling_interval);
            }
        }
        
        value=getenv("BLENGTH");
        if (value == NULL || value[0]=='\0')
            burst_length=BURST_LENGTH;
        else {
            burst_length=strtoul(value, NULL, 0);
            if (burst_length==0) {
                burst_length=BURST_LENGTH;
                printf("[profiler] WARNING: invalid value specified for BLENGTH, using default (%lu) instead\n", burst_length);
            }
        }        
                
        // initialize also TLS fields for calling thread
        shadow_stack_idx=0;
        aligned=1;
		exhaustive_enter_events=0;	
				
		if (__real_pthread_create(&timerThreadID, NULL, timerThread, NULL)) {
            printf("[profiler] error creating timer thread - exiting!\n");
            exit(1);
        }
}
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
		
        #if SHOW_MESSAGES==1
        pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program start - tid %d\n", tid);
        #endif              
                        
        hcct_init();
        
}

// execute after termination
void __attribute__((destructor, no_instrument_function)) trace_end(void)
{
		// close timer thread
		#if SHOW_MESSAGES==1
        printf("[profiler] closing timer thread...\n");
        #endif        
        if (pthread_cancel(timerThreadID))
            printf("[profiler] WARNING: could not close timer thread\n");

		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program exit - tid %d\n", tid);
        #endif
        
        #if DUMP_TREE==1 && PROFILER_EMPTY==0
		hcct_dump_map();		
		#endif
        
        hcct_dump();
}

// routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	++exhaustive_enter_events;
				
	shadow_stack[shadow_stack_idx].routine_id=(unsigned long)this_fn;
	shadow_stack[shadow_stack_idx++].call_site=(unsigned long)call_site;
	
	if (burst_on==0) aligned=0;
	else {
		if (sampling_interval==0) hcct_init();
        if (aligned==0) hcct_align();
        else hcct_enter((unsigned long)this_fn, (unsigned long)call_site);
    }
}

// routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	    
    --shadow_stack_idx;
                
    if (burst_on==0) aligned=0;
	else {
        // aligning is not needed (no info provided for tree update)
        if (aligned==1) hcct_exit();
    }                                        
}

// wrap pthread_exit()
void __attribute__((no_instrument_function)) __wrap_pthread_exit(void *value_ptr)
{
		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
		printf("[profiler] pthread_exit - tid %d\n", tid);
		#endif
		
		hcct_dump();
        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                
        // initialize TLS
        shadow_stack_idx=0;
        aligned=1;
		exhaustive_enter_events=0;
        
        hcct_init();

        // retrieve original routine address and argument        
        void* orig_arg=((void**)arg)[1];        
        void *(*start_routine)(void*)=((void**)arg)[0];
        free(arg);
        
		// run actual application thread's init routine
        void* ret=(*start_routine)(orig_arg);

		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
        printf("[profiler] return - tid %d\n", tid);
        #endif
                
        hcct_dump();
        
        return ret;
}

// wrap pthread_create
int __attribute__((no_instrument_function)) __wrap_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
        void** params=malloc(2*sizeof(void*));
        params[0]=start_routine;
        params[1]=arg;
        return __real_pthread_create(thread, attr, aux_pthread_create, params);
}
