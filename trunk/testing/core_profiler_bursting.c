#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
//~ #include <signal.h>
#include <time.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t, timer_t

// timer globals
pthread_t       timerThreadID;
unsigned short  burst_on;   // enable or disable bursting
unsigned long   sampling_interval;
unsigned long   burst_length;

#define BURSTING 1
#include "common.h"

// shadow stack - legal values from 0 up to shadow_stack_idx-1
__thread hcct_stack_node_t  shadow_stack[STACK_MAX_DEPTH];
__thread int                shadow_stack_idx; // PER ORA USO INT PER VERIFICARE SE SUCCEDONO COSE STRANE - DA SISTEMARE
__thread int                aligned;


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
            //~ printf("Disabling bursting\n");
            burst_on=0;
            clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &disabled, NULL);
        } else {
            //~ printf("Enabling bursting\n");
            burst_on=1;
            clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &enabled, NULL);
        }
    }
}
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        printf("[profiler] program start - tid %d\n", syscall(__NR_gettid));
        #endif
        
        // Initializing timer thread parameters (granularity: nanoseconds)
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

        // conviene passare burston come parametro?
        if (__real_pthread_create(&timerThreadID, NULL, timerThread, NULL)) {
            printf("[profiler] error creating timer thread - exiting!\n");
            exit(1);
        }        
        
        // Initializing analysis algorithm parameters
        if (hcct_getenv()!=0) {
            printf("[profiler] error getting parameters - exiting...\n");
            exit(1);   
        }
        
        // Initialize shadow stack
        shadow_stack_idx=0;
        aligned=1;
        
        // Initializing hcct module        
        if (hcct_init()==-1) {
            printf("[profiler] error during initialization - exiting...\n");
            exit(1);   
        }
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		// Close timer thread
		#if SHOW_MESSAGES==1
        printf("[profiler] closing timer thread...\n");
        #endif        
        if (pthread_cancel(timerThreadID))
            printf("[profiler] WARNING: could not close timer thread\n");

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
	
	// Shadow stack
	shadow_stack[shadow_stack_idx].routine_id=(unsigned long)this_fn;
	shadow_stack[shadow_stack_idx++].call_site=cs;
	
	if (burst_on==0) aligned=0;
	else {
        if (aligned==0) hcct_align();
        else hcct_enter((unsigned long)this_fn, cs);
    }
}

// Routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
    // Shadow stack
    --shadow_stack_idx;
        
    //~ if (--shadow_stack_idx<0) {
    //~ printf("FATAL ERROR: stack index < 0\n");
    //~ exit(1);
    //~ }        
                
    if (burst_on==0) aligned=0;
	else {
        // Aligning is not needed (no info provided for tree update)
        if (aligned==1) hcct_exit();
    }                                        
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
        // Initialize shadow stack
        shadow_stack_idx=0;
        aligned=1;
        
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
