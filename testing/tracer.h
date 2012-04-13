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

#ifndef TRACER_H
#define TRACER_H

#include "common.h"
#include <stdio.h>
#include <pthread.h>

#define MAGIC_NUMBER    0xABADCAFE
#define BLOCK_SIZE      BUFSIZ      // from stdio.h
#define OPTIMIZE_IO     1
#define GRANULARITY		200000
#define BSIZE_MAIN		100000000
#define BSIZE_THREAD	10000000

typedef struct TraceWriter TraceWriter;
struct TraceWriter {    
    // Internals
    FILE* out;
    UINT32 bufferSize, used[2]; // up to 4 GB
    void *buffer[2]; // myWriter uses buffer[0]
    int index;        
    UINT64 firstTick, ticks;
    pthread_t dumpThread;
    
    // Stream stats
    UINT64 rtnEnterEvents;
    UINT64 rtnExitEvents;
};

// MEM_READ and MEM_WRITE are not implemented
enum EVENT_TYPE {
    RTN_ENTER       = 0x0,
    RTN_EXIT        = 0x1, 
    MEM_READ        = 0x2,
    MEM_WRITE       = 0x3,
    TIMER_TICK      = 0x4,
    RTN_ENTER_CS    = 0x5
};

#endif
