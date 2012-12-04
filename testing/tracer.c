// ==================================================================
// This tool supports the creation of a binary output file recording
// a stream of events. The file has the following format:
//      magic number (4 bytes)
//      number of routine enter events (8 bytes)
//      number of routine exit events (8 bytes)
//      number of timer ticks (8 bytes)
//      #ticks since application started (8 bytes)
//      timer granularity in microseconds (4 bytes)
//      stream of events (rtn enter, rtn exit, timer tic)
// ==================================================================

#define _GNU_SOURCE // fopen64 (I guess)	

#include "tracer.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h> // getcwd

#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t

#include <errno.h>
extern char *program_invocation_short_name;

// Global variables and TLS
UINT64 					timer;
UINT32					timerGranularity;
pthread_t				timerThreadID;
__thread TraceWriter	*trace;

// TODO: save process memory map

#if OPTIMIZE_IO==0
// Use standard fwrite
#define myWrite(ptr, size, nitems, tr) fwrite(ptr,size,nitems,tr->out)
#else
// TraceWriter buffering schema
void __attribute__((no_instrument_function)) dumpToFile(void* arg) {
    TraceWriter *tr=(TraceWriter*)arg;
    
    int index=(tr->index-1)%2; // incremented before pthread_create()
    char* buffer=(char*)tr->buffer[index];    
    
    #if 1
    if (fwrite(buffer, tr->used[index], 1, tr->out)!=1) {
        printf("[tracer] Error %d writing to disk! Exiting...\n", errno);
        exit(EXIT_FAILURE);
    } else tr->used[index]=0;
    #else
    int ret;
    unsigned block=BLOCK_SIZE;
    
    while (tr->used[index]!=0) {
        if (tr->used[index]<block) block=tr->used[index];
        ret=fwrite(buffer, 1, block, tr->out);
        if (ret==-1) {
            if (errno==EINTR) continue;
            else {
                printf("[tracer] Error %d writing to disk! Exiting...\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        tr->used[index]-=ret;
        buffer+=ret;
    }
    #endif    
    __real_pthread_exit(NULL);
}

// used by TraceWriter_Close()
void __attribute__((no_instrument_function)) mySync(TraceWriter* tr) {
    if (tr->index > 0)
		pthread_join(tr->dumpThread, NULL);
    
    // Synchronous I/O
    int index=tr->index%2; // buffer currently in use
    char* buffer=(char*)tr->buffer[index];
    
    #if 1
    if (fwrite(buffer, tr->used[index], 1, tr->out)!=1) {
        printf("[tracer] Error %d writing to disk! Exiting...\n", errno);
        exit(EXIT_FAILURE);
    } else tr->used[index]=0;
    #else
    int ret;
    unsigned block=BLOCK_SIZE;
    
    while (tr->used[index]!=0) {
        if (tr->used[index]<block) block=tr->used[index];
        ret=fwrite(buffer, 1, block, tr->out);
        if (ret==-1) {
            if (errno==EINTR) continue;
            else {
                printf("[tracer] Error %d writing to disk! Exiting...\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        tr->used[index]-=ret;
        buffer+=ret;
    }
    #endif
}

void __attribute__((no_instrument_function)) myWrite(const void *ptr, size_t size, size_t nitems, TraceWriter *tr) {
    int sizeTot=size*nitems;
    if (sizeTot + tr->used[tr->index%2] > tr->bufferSize) {
        if (tr->index>0)
			pthread_join(tr->dumpThread, NULL);
        tr->index++;      
        if (__real_pthread_create(&(tr->dumpThread), NULL, dumpToFile, tr)) {
            printf("[tracer] Can't spawn internal thread! Exiting...");
            exit(EXIT_FAILURE);
        }        
    }    
    memcpy((char*)tr->buffer[tr->index%2]+tr->used[tr->index%2], ptr, sizeTot);	    
    tr->used[tr->index%2]+=sizeTot;
}
#endif

// Counts how many ticks have been added since last check
#define checkTimer(tr) if (tr->ticks!=timer) { \
		typeByte = TIMER_TICK; \
		int i; \
		for (i=timer-tr->ticks; i>0; --i, tr->ticks++) \
			myWrite(&typeByte, sizeof(char), 1, tr); \
		}

// TraceWriter creation function
TraceWriter* __attribute__((no_instrument_function)) TraceWriter_Open(UINT32 bufferSize){

    TraceWriter *tr = (TraceWriter *) malloc(sizeof(TraceWriter));
    if (tr == NULL) return NULL;
    
    // up to 10 digits for PID on 64 bits systems - however check /proc/sys/kernel/pid_max
    pid_t tid=syscall(__NR_gettid);	    
    char* traceFileName=malloc(strlen(program_invocation_short_name)+18); // suffix: -PID.trace\0
    sprintf(traceFileName, "%s-%d.trace", program_invocation_short_name, tid);
    tr->out=fopen64(traceFileName, "wb+");
    
    if (tr->out == NULL) {
        free(tr);
	    return NULL;
    }
    
    tr->bufferSize=bufferSize;
    tr->buffer[0]=malloc(tr->bufferSize);
    if (tr->buffer[0]==NULL) {
        free(tr);
        return NULL;
    }
    
    tr->buffer[1]=malloc(tr->bufferSize);
    if (tr->buffer[1]==NULL) {
        free(tr->buffer[0]);
        free(tr);
        return NULL;
    }
    
    tr->index=0;
    tr->used[0]=0;
    tr->used[1]=0;
    
    tr->ticks=timer;
    tr->firstTick=tr->ticks;
    
    // write magic number
    unsigned long magic = MAGIC_NUMBER;
    myWrite(&magic, sizeof(unsigned long), 1, tr);

    // skip space for file info
    unsigned long noValue = 0;
    int i;
    for (i=0; i<9; i++) // 36 bytes (4 UINT64 + 1 UINT32)
		myWrite(&noValue, sizeof(unsigned long), 1, tr);
   
    // init event counters
    tr->rtnEnterEvents = 0;
    tr->rtnExitEvents  = 0;     
    
    return tr;
}

// TraceWriter delete function
void __attribute__((no_instrument_function)) TraceWriter_Close(TraceWriter* tr){
    #if OPTIMIZE_IO
    mySync(tr);
    free(tr->buffer[0]);
    free(tr->buffer[1]);
    #endif    

    unsigned long long tmp=tr->ticks - tr->firstTick; // number of ticks since thread start
    
    if ( fseeko(tr->out, (off_t) sizeof(unsigned long), SEEK_SET) == -1 ) {
		printf("[tracer] Error: could not perform fseeko() on output file! Exiting...\n", errno);
        exit(EXIT_FAILURE);		
	}
    
    fwrite(&tr->rtnEnterEvents, sizeof(unsigned long long), 1, tr->out); 
    fwrite(&tr->rtnExitEvents, sizeof(unsigned long long), 1, tr->out);    
    fwrite(&tmp, sizeof(unsigned long long), 1, tr->out);
    fwrite(&tr->firstTick, sizeof(unsigned long long), 1, tr->out);
    fwrite(&timerGranularity, sizeof(unsigned long), 1, tr->out);
    
    fclose(tr->out);
    free(tr);
}

// separate thread to handle timer
void __attribute__((no_instrument_function)) timerThread(void* arg)
{
	struct timespec ts;
	ts.tv_sec=0;
	ts.tv_nsec=GRANULARITY;
    while(1)
    {
		timer++;
	    clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &ts, NULL);
    }
}

// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
		// Create timer thread
		if (__real_pthread_create(&timerThreadID, NULL, timerThread, NULL)) {
            printf("[tracer] error creating timer thread - exiting!\n");
            exit(1);
        }        
        
        trace=TraceWriter_Open(BSIZE_MAIN);        
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		// Close timer thread
		if (pthread_cancel(timerThreadID))
			printf("[tracer] WARNING: could not close timer thread\n");
				
		TraceWriter_Close(trace);
}

// Routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	char typeByte;
    trace->rtnEnterEvents++;
    
    checkTimer(trace);
    
    // Write rtn_enter_cs event (7 bytes)
    typeByte = RTN_ENTER_CS;
    myWrite(&typeByte, sizeof(char), 1, trace); // 1 byte for event type            
    myWrite(&this_fn, sizeof(void*), 1, trace); // 4 bytes for routine address
    myWrite(&call_site, sizeof(void*)/2, 1, trace); // 2 bytes for call site (16 LSBs)
}

// Routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
    char typeByte;
    trace->rtnExitEvents++;
    
    checkTimer(trace);
    
    typeByte = RTN_EXIT;
    myWrite(&typeByte, sizeof(char), 1, trace);    
}

// wrap pthread_exit()
void __attribute__((no_instrument_function)) __wrap_pthread_exit(void *value_ptr)
{
		TraceWriter_Close(trace);
		
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                
        trace=TraceWriter_Open(BSIZE_THREAD);

        // Retrieve original routine address and argument        
        void* orig_arg=((void**)arg)[1];        
        void *(*start_routine)(void*)=((void**)arg)[0];
        free(arg);
        
		// Run actual application thread's init routine
        void* ret=(*start_routine)(orig_arg);
		        
        TraceWriter_Close(trace);
        
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
