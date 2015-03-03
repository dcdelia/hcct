#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// #include <asm/unistd.h> // __NR_gettid

#define NUM_THREADS 8

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
        
        if (tid%2==0) {
			test1();
			test2();
			test1();
			test3();
			test2();
		} else {
			test3();
			test2();
		}
        
        if (tid%2==0) { // tests two different thread termination methods (return and pthread_exit)
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
