#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t, timer_t

// timer globals
pthread_t       timerThreadID;
unsigned short  burst_on;   // enable or disable bursting
unsigned long   sampling_interval;
unsigned long   burst_length;

unsigned long   no_burst_length; // not needed anymore...??


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

// separate thread to handle timer
void __attribute__((no_instrument_function)) timerThread(void* arg)
{
    struct timespec enabled, disabled;
    enabled.tv_sec=0;
    enabled.tv_nsec=burst_length;
    disabled.tv_sec=0;
    disabled.tv_nsec=sampling_interval-burst_length;
    
    while(1)
    {
        if (burst_on!=0) {
            //printf("Disabling bursting\n");
            burst_on=0;
            clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &disabled, NULL);
        } else {
            //printf("Enabling bursting\n");
            burst_on=1;
            clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &enabled, NULL);
        }
    }
    
    // TODO: inserire gestore segnale terminazione :)
    
}
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        printf("[profiler] program start - tid %d\n", syscall(__NR_gettid));
        #endif
        
        // Initializing timer (granularity: nanoseconds)
        burst_on=1;
        sampling_interval=10*1000000;
        burst_length=1*1000000;

        // conviene passare burston come parametro?
        if (__real_pthread_create(&timerThreadID, NULL, timerThread, NULL)) {
            printf("[profiler] error creating timer thread - exiting!\n");
            exit(1);
        }        
        
        // Create a new thread and run clock_nanosleep inside it
                
        // Initializing user parameters
        if (hcct_getenv()!=0) {
            printf("[profiler] error getting parameters - exiting...\n");
            exit(1);   
        }
        
        // Initializing hcct module        
        if (hcct_init()==-1) {
            printf("[profiler] error during initialization - exiting...\n");
            exit(1);   
        }
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		// Shut down timer
		#if 0
		if (timer_delete(global_timer))
		    printf("[profiler] WARNING: timer_delete error\n");
        #else
        if (pthread_kill(timerThreadID, SIGTERM))
            printf("[profiler] WARNING: could not kill timer thread\n");
        #endif

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
