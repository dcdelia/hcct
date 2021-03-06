#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <string.h>

#define _GNU_SOURCE
#include <errno.h>
extern char *program_invocation_short_name;

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
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
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
		hcct_enter((unsigned long)this_fn, (unsigned long)call_site);
}

// routine exit
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
				
		hcct_dump();
        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                                
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
        #if SHOW_MESSAGES==1
        pid_t tid=syscall(__NR_gettid);
        printf("[profiler] spawning a new thread from tid %d\n", tid);
        #endif
        
        void** params=malloc(2*sizeof(void*));
        params[0]=start_routine;
        params[1]=arg;        
        return __real_pthread_create(thread, attr, aux_pthread_create, params);
}
