#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <asm/unistd.h> // per la costante di chiamata __NR_gettid
#define NUM_THREADS 8

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

void test3() {
	int a=90;
	a++;
}

void test2() {
	int x=5;
	x--;
	test3();
}

void test1() {
	int a=4;
	for (;a>0;a--)
		test2();
	test3();
}

void *TaskCode(void *argument) 
{
	int tid;
	tid = *((int *) argument);
	//printf("Hello World! It's me, thread %d aka %lu! Setting tls_local_data to %lu..\n", tid, (unsigned long)syscall(__NR_gettid), tls_local_data);
	
	test1();
	test2();
	test1();
	test3();
	test2();
	
	if (tid%2==0) { // per provare due modi diversi di terminare thread (return o pthread_exit)
		pthread_exit(NULL);
	}
	else return NULL;
}

int main(void)
{
	pthread_t threads[NUM_THREADS];
	int thread_args[NUM_THREADS];
	int rc, i;

	//printf("%lu\n", (unsigned long)TaskCode);

	/* create all threads */
	for (i=0; i<NUM_THREADS; ++i) {
		thread_args[i] = i;
		printf("In main: creating thread %d\n", i);
		rc = pthread_create(&threads[i], NULL, TaskCode, (void *) &thread_args[i]);
		assert(0 == rc);
	}

	/* wait for all threads to complete */
	for (i=0; i<NUM_THREADS; ++i) {
		rc = pthread_join(threads[i], NULL);
		assert(0 == rc);
	}

	exit(EXIT_SUCCESS);
}
