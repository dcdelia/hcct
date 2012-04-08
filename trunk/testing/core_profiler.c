#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t

// Eliminare questa porcheria!!! :)
#ifdef PROFILER_CCT
#include "cct.h"
#else
#ifdef PROFILER_EMPTY
#include "empty.h"
#else
#include "lss-hcct.h"
#endif
#endif
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        printf("[profiler] program start - tid %d\n", syscall(__NR_gettid));
        #endif
        
        if (hcct_getenv()!=0) {
            printf("[profiler] error getting parameters - exiting...\n");
            exit(1);   
        }
                
        if (hcct_init()==-1) {
            printf("[profiler] error during initialization - exiting...\n");
            exit(1);   
        }
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		#if SHOW_MESSAGES==1
        printf("[profiler] program exit - tid %d\n", syscall(__NR_gettid));
        #endif
        hcct_dump(hcct_get_root(), 1);
}

// Routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	unsigned short cs= (unsigned short)(((unsigned long)call_site)&(0xFFFF)); // PLEASE CHECK THIS
 
	// Nel TraceWriter veniva fatto così
	/*typeByte = RTN_ENTER_CS;
	// 2 bytes for call site (16 LSBs)
	myWrite(&ip, sizeof(ADDRINT)/2, 1, tr);*/
	
	hcct_enter((unsigned long)this_fn, cs);
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
		printf("[profiler] pthread_exit - tid %d\n", syscall(__NR_gettid));
		#endif
		
		// Exit stuff
		hcct_dump(hcct_get_root(), 1);
        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                
        hcct_init();

        // Retrieve original routine address and argument        
        void* orig_arg=((void**)arg)[1];        
        void *(*start_routine)(void*)=((void**)arg)[0];
        free(arg);
        
		// Run actual application thread's init routine
        void* ret=(*start_routine)(orig_arg);

		#if SHOW_MESSAGES==1
        printf("[profiler] return - tid %d\n", syscall(__NR_gettid));
        #endif
        
        // Exit stuff
        hcct_dump(hcct_get_root(), 1);
        
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
