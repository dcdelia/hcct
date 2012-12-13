#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h> // syscall()
#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t, timer_t

#include <string.h>
#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_short_name;

#define PROFILE_TIME	1
#define TIMER_TYPE		CLOCK_REALTIME
#include "common.h"

// timer globals
pthread_t       timerThreadID;
unsigned long   sampling_interval;
UINT64			global_tics;

// TLS
__thread UINT64 thread_tics;
__thread UINT64	enter_events;

// shadow stack - legal values from 0 up to shadow_stack_idx-1
__thread hcct_stack_node_t  shadow_stack[STACK_MAX_DEPTH];
__thread int                shadow_stack_idx; // TODO

// TODO
#ifdef PROFILER_CCT
#include "cct.h"
//~ #else
//~ #ifdef PROFILER_EMPTY
//~ #include "empty.h"
//~ #else
//~ #include "lss-hcct.h"
//~ #endif
#endif

#if DUMP_TREE==1
	#ifndef PROFILER_EMPTY
	static void __attribute__ ((no_instrument_function)) hcct_dump_map() {
	
		pid_t pid=syscall(__NR_getpid);
		
		// Command to be executed: "cp /proc/<PID>/maps <name>.map\0" => 9+10+6+variable+5 bytes needed
		char command[BUFLEN+1];
		sprintf(command, "cp -f /proc/%d/maps %s.map && chmod +w %s.map", pid, program_invocation_short_name, program_invocation_short_name);
	
		int ret=system(command);
		if (ret!=0) printf("[profiler] WARNING: unable to read currently mapped memory regions from /proc\n");	
	
	}
	#endif
#endif


// timerThread
void __attribute__((no_instrument_function)) timerThread(void* arg)
{
	struct timespec global_timer;
	global_timer.tv_sec=0; // TODO
	global_timer.tv_nsec=sampling_interval;	
	
    while(1) {
		global_tics++;
		clock_nanosleep(TIMER_TYPE, 0, &global_timer, NULL);
		#if SHOW_MESSAGES==1
		//~ printf("[profiler] timer tic\n");
		#endif
	}
}
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program start - tid %d\n", tid);
        #endif              
        
        // Initializing global parameters
        global_tics=0;        		
		
		// Initialize TLS
		thread_tics=0;
        shadow_stack_idx=0;
        enter_events=0;     
        
        char* value;
        value=getenv("SINTVL");
        if (value == NULL || value[0]=='\0')
            sampling_interval=PROFILE_TIME_INTVL;
        else {
            sampling_interval=strtoul(value, NULL, 0);
            if (sampling_interval==0) {
                sampling_interval=PROFILE_TIME_INTVL;
                printf("[profiler] WARNING: invalid value specified for SINTVL, using default (%lu) instead\n", sampling_interval);
            }
        }

        // Initializing analysis algorithm parameters
        if (hcct_getenv()!=0) {
            printf("[profiler] error getting parameters - exiting...\n");
            exit(1);   
        }
                
		// Start timer thread       
        if (__real_pthread_create(&timerThreadID, NULL, timerThread, NULL)) {
            printf("[profiler] error creating timer thread - exiting!\n");
            exit(1);
        }        
        
        // Initializing hcct module        
        if (hcct_init()==-1) {
            printf("[profiler] error during initialization - exiting...\n");
            exit(1);   
        }
        
}

// execute after termination
void __attribute__((destructor, no_instrument_function)) trace_end(void)
{
		// Close timer thread
		#if SHOW_MESSAGES==1
        printf("[profiler] closing timer thread...\n");
        #endif        
        if (pthread_cancel(timerThreadID))
            printf("[profiler] WARNING: could not close timer thread\n");

		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
        printf("[profiler] program exit - tid %d\n", tid);
        #endif
        
        #if DUMP_TREE==1
			#ifndef PROFILER_EMPTY
			hcct_dump_map();
			#endif
		#endif

        hcct_dump();
}

// Routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	//~ #if SHOW_MESSAGES==1
	//~ printf("[profiler] entering routine %lX - shadow_stack_idx is %d\n", (ADDRINT)this_fn, shadow_stack_idx);
	//~ #endif
	
	if (global_tics>thread_tics) {
		UINT32 increment = global_tics - thread_tics;
		thread_tics=global_tics;
		#if SHOW_MESSAGES==1
		//~ printf("[hcct] perform aligning\n");
		#endif		
		hcct_align(increment);
	}
	
	++enter_events;
			
	// Shadow stack
	shadow_stack[shadow_stack_idx].routine_id=(unsigned long)this_fn;
	shadow_stack[shadow_stack_idx++].call_site=(unsigned long)call_site;		
}

// Routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
	//~ #if SHOW_MESSAGES==1
	//~ printf("[profiler] leaving routine %lX - shadow_stack_idx is %d\n", (ADDRINT)this_fn, shadow_stack_idx);
	//~ #endif
		
	if (global_tics>thread_tics) {
		UINT32 increment = global_tics - thread_tics;
		thread_tics=global_tics;		
		#if SHOW_MESSAGES==1
		//~ printf("[hcct] perform aligning\n");
		#endif				
		hcct_align(increment);
	}
	
    // Shadow stack
    --shadow_stack_idx;
    
    // Note that hcct_exit() is not needed!
                    
}

// wrap pthread_exit()
void __attribute__((no_instrument_function)) __wrap_pthread_exit(void *value_ptr)
{
		#if SHOW_MESSAGES==1
		pid_t tid=syscall(__NR_gettid);
		printf("[profiler] pthread_exit - tid %d\n", tid);
		#endif
		
		// Exit stuff
		hcct_dump();
        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                
        // Initialize TLS
        shadow_stack_idx=0;
        thread_tics=0;
		enter_events=0;
        	
        // Initialize analysis module
        hcct_init();

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
        
        // Exit stuff
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
