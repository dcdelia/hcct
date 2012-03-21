#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <asm/unistd.h> // per la costante di chiamata __NR_gettid

#include "cct.h"

// To compile:
// gcc core_profiler.c cct.c -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -o core_profiler -lpthread

__thread unsigned long tls_local_data;
 
// execute before main
void __attribute__ ((constructor)) trace_begin (void) __attribute__((no_instrument_function));
void __attribute__ ((constructor)) trace_begin (void)
{
        printf("Inizio programma\n");
        tls_local_data=(unsigned long)pthread_self();
        cct_init();
}

// execute after termination
void __attribute__ ((destructor)) trace_end (void) __attribute__((no_instrument_function));
void __attribute__ ((destructor)) trace_end (void)
{
        printf("Uscita dal programma\n");
        cct_dump(cct_get_root(), 1);
}

// Routine enter
void __cyg_profile_func_enter(void *this_fn, void *call_site)
                              __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  //printf("ENTER: %p, from %p in thread %lu\n", this_fn, call_site, tls_local_data);
  //printf("ENTER: %p, from %p in thread %lu\n", this_fn, call_site, (unsigned long)pthread_self());
  
  unsigned short cs= (unsigned short)(((unsigned long)call_site)&(0xFFFF)); // PLEASE CHECK THIS
  //printf("test call site: %lu, %hu\n", (unsigned long)this_fn, cs);
 
   /*typeByte = RTN_ENTER_CS;
    // 2 bytes for call site (16 LSBs)
    myWrite(&ip, sizeof(ADDRINT)/2, 1, tr);*/
        cct_enter((unsigned long)this_fn, cs);
}

// Routine exit
void __cyg_profile_func_exit(void *this_fn, void *call_site)
                             __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  //printf("EXIT:  %p, from %p in thread %lu\n", this_fn, call_site, tls_local_data);
  //printf("EXIT:  %p, from %p in thread %lu\n", this_fn, call_site, (unsigned long)pthread_self());
        cct_exit();
}


// routine di exit
void uscita() __attribute__((no_instrument_function));
void uscita() {
        //printf("Sto uscendo da questo thread... %d\n", tls_local_data); fflush(0);
        cct_dump(cct_get_root(), 1);
}


// TODO: VEDI VARIE VERSIONI PTHREAD_EXIT con o senza args
// intercept pthread_exit()
void __wrap_pthread_exit(void *value_ptr) __attribute__((no_instrument_function));
void __wrap_pthread_exit(void *value_ptr)
{
        //printf("Uscita regolare, eri nel thread con tls %lu!\n", tls_local_data); fflush(0);
        uscita();
        __real_pthread_exit(value_ptr);
}

// intercept pthread_create to change destructor
void* aux_routine(void *arg) __attribute__((no_instrument_function));
void* aux_routine(void *arg) {
        tls_local_data=(unsigned long)pthread_self();
        void** tmp=(void**)arg;
        void* orig_routine=tmp[0];
        void* orig_arg=tmp[1];
        free(tmp);
        //printf("Here!\n");
        //printf("%lu\n", (unsigned long)orig_routine);
        
        cct_init();
        
        void *(*start_routine)(void*)=orig_routine;
        void* ret=(*start_routine)(orig_arg);

        //printf("Intercettato uscita senza pthread_exit da %lu...\n", tls_local_data); fflush(0);
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
