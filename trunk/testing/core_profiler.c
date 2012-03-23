#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t

#ifdef PROFILER_CCT
#include "cct.h"
#else
#ifdef PROFILER_EMPTY
#include "empty.h"
#else
#include "lss-hcct.h"
#endif
#endif

//__thread unsigned long tls_local_data;
__thread pid_t hcct_thread_id;
 
// execute before main
void __attribute__ ((constructor)) trace_begin (void) __attribute__((no_instrument_function));
void __attribute__ ((constructor)) trace_begin (void)
{
		//tls_local_data=(unsigned long)pthread_self();
        hcct_thread_id=syscall(__NR_gettid);
        printf("[profiler] program start - tid %d\n", hcct_thread_id);
                
        hcct_init();
}

// execute after termination
void __attribute__ ((destructor)) trace_end (void) __attribute__((no_instrument_function));
void __attribute__ ((destructor)) trace_end (void)
{
        printf("[profiler] program exit - tid %d\n", hcct_thread_id);
        hcct_dump(hcct_get_root(), 1);
}

// Routine enter
void __cyg_profile_func_enter(void *this_fn, void *call_site)
                              __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	
	unsigned short cs= (unsigned short)(((unsigned long)call_site)&(0xFFFF)); // PLEASE CHECK THIS
 
   /*typeByte = RTN_ENTER_CS;
	// 2 bytes for call site (16 LSBs)
	myWrite(&ip, sizeof(ADDRINT)/2, 1, tr);*/
	
	hcct_enter((unsigned long)this_fn, cs);
}

// Routine exit
void __cyg_profile_func_exit(void *this_fn, void *call_site)
                             __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
        hcct_exit();
}


// routine di exit
void uscita() __attribute__((no_instrument_function));
void uscita() {
        //printf("Sto uscendo da questo thread... %d\n", tls_local_data); fflush(0);
        hcct_dump(hcct_get_root(), 1);
}


// TODO: VEDI VARIE VERSIONI PTHREAD_EXIT con o senza args
// intercept pthread_exit()
void __wrap_pthread_exit(void *value_ptr) __attribute__((no_instrument_function));
void __wrap_pthread_exit(void *value_ptr)
{
		printf("[profiler] pthread_exit - tid %d\n", hcct_thread_id);
        uscita();
        __real_pthread_exit(value_ptr);
}

// intercept pthread_create to change destructor
void* aux_routine(void *arg) __attribute__((no_instrument_function));
void* aux_routine(void *arg)
{
        //tls_local_data=(unsigned long)pthread_self();
        hcct_thread_id=syscall(__NR_gettid);
         
        hcct_init();

		// Una porcheria :)
		void** tmp=(void**)arg;
        void* orig_routine=tmp[0];
        void* orig_arg=tmp[1];
        free(tmp);        
        void *(*start_routine)(void*)=orig_routine;

		// Lancia routine originaria per thread start
        void* ret=(*start_routine)(orig_arg);

        printf("[profiler] return - tid %d\n", hcct_thread_id);
        
        uscita();
        return ret;
}

int __wrap_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg) __attribute__((no_instrument_function));
int __wrap_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
        void** params=malloc(2*sizeof(void*));
        params[0]=start_routine;
        params[1]=arg;
        return __real_pthread_create(thread, attr, aux_routine, params);
}
