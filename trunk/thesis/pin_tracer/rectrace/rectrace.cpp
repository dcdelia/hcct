// ===============================================================
// This tool creates a binary output file with a stream of events
// ===============================================================

// Author:       Irene Finocchi
// Last changed: $Date: 2010/11/09 10:16:46 $
// Changed by:   $Author: pctips $
// Revision:     $Revision: 1.12 $

// TODO: check makefile

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include "STool.h"
#include "TraceWriter.h"
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>
#include <unistd.h>

// =============================================================
// Constants and macros
// =============================================================

#define BSIZE_MAIN  100000000
#define BSIZE_OTHER 10000000
#define LOGTIME 1
#define EXTERNAL_TIMER 0
#define DUMP_NAMES 0

#if LOGTIME
#define TIMER_GRANULARITY 200
#if EXTERNAL_TIMER
static UINT64 *ptimer;
#include <sys/ipc.h>
#include <sys/shm.h> 
#else
static UINT64 timer;
static UINT64 *ptimer=&timer;
#endif
#endif

// =============================================================
// Structures
// =============================================================

// Thread local data record
typedef struct ThreadRec {
    TraceWriter* tw;
    BOOL logTime;
} ThreadRec;


// =============================================================
// Utils
// =============================================================
#if LOGTIME && EXTERNAL_TIMER==0
static VOID timerThread(VOID* arg) {
    while(1) {
        if (PIN_IsProcessExiting()) PIN_ExitThread(0);
        timer++;
        usleep(TIMER_GRANULARITY);
        //PIN_Sleep(TIMER_GRANULARITY/1000); // granularity: ms
    }
}
#endif

unsigned long long Get_time() { // returns the number of microseconds elapsed since the epoch
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long t = ((unsigned long long)tv.tv_sec) * 1000000 + (unsigned long long)tv.tv_usec;
	return t;
}

// =============================================================
// Callback functions
// =============================================================

// -------------------------------------------------------------
// Thread start function
// -------------------------------------------------------------
void ThreadStart(ThreadRec* t) {
    char fileName[64];

    // make output file name
    #if CALL_SITE
    sprintf(fileName, "%s-%u.cs.trace", STool_StripPath(STool_AppName()), STool_ThreadID(t));
    #else
    sprintf(fileName, "%s-%u.trace", STool_StripPath(STool_AppName()), STool_ThreadID(t));
    #endif

    // print message
    printf("[TID=%u] Starting new thread with output file %s\n", STool_ThreadID(t), fileName);

    // create tracewriter object
    if (STool_ThreadID(t)==0)
        #if LOGTIME
        t->tw = TraceWriter_Open(fileName, ptimer, TIMER_GRANULARITY, BSIZE_MAIN);
        #else
        t->tw = TraceWriter_Open(fileName, NULL, 0, BSIZE_MAIN);
        #endif
    else
        #if LOGTIME
        t->tw = TraceWriter_Open(fileName, ptimer, TIMER_GRANULARITY, BSIZE_OTHER);
        #else
        t->tw = TraceWriter_Open(fileName, NULL, 0, BSIZE_OTHER);
        #endif
    if (t->tw == NULL) exit((printf("Can't create tracewriter object with output file %s\n", fileName), 1));

}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
void ThreadEnd(ThreadRec* t) {
    // delete tracewriter object
    if (TraceWriter_Close(t->tw) == false) exit((printf("Can't delete tracewriter object\n"), 1));
}


// -------------------------------------------------------------
// Routine enter analysis function
// -------------------------------------------------------------
static void RoutineEnter(ThreadRec* t
                        #if CALL_SITE
                        , ADDRINT ip
                        #endif
                        ) {

    ADDRINT target = STool_CurrRoutine(t);

    #if CALL_SITE
    
    #if DUMP_NAMES
    TraceWriter_RtnEnter(t->tw, target, ip, STool_RoutineNameByAddr(target), STool_LibraryNameByAddr(target));
    #else
    TraceWriter_RtnEnter(t->tw, target, ip);
    #endif
    
    #else
    
    #if DUMP_NAMES
    TraceWriter_RtnEnter(t->tw, target, STool_RoutineNameByAddr(target), STool_LibraryNameByAddr(target));
    #else
    TraceWriter_RtnEnter(t->tw, target);
    #endif
    
    #endif
}

// -------------------------------------------------------------
// Routine exit analysis function
// -------------------------------------------------------------
static void RoutineExit(ThreadRec* t) {

    TraceWriter_RtnExit(t->tw);
}

#if TRACE_MEMORY_ACCESSES
// -------------------------------------------------------------
// Memory read analysis function
// -------------------------------------------------------------
static void MemoryRead(ThreadRec* t, ADDRINT addr) {

    TraceWriter_MemRead(t->tw, addr);
}

// -------------------------------------------------------------
// Memory write analysis function
// -------------------------------------------------------------
static void MemoryWrite(ThreadRec* t, ADDRINT addr) {

    TraceWriter_MemWrite(t->tw, addr);
}
#endif

// =============================================================
// Main function of the analysis tool
// =============================================================

int main(int argc, char* argv[]) {
    
    STool_TSetup setup = { 0 };

    setup.argc          = argc;
    setup.argv          = argv;
    setup.threadRecSize = sizeof(ThreadRec);
    setup.threadStart   = (STool_TThreadH)ThreadStart;
    setup.threadEnd     = (STool_TThreadH)ThreadEnd;
    setup.rtnEnter      = (STool_TRtnH)RoutineEnter;
    setup.rtnExit       = (STool_TRtnH)RoutineExit;
    
    #if CALL_SITE
    setup.callingSite   = 1; // TRUE :)
    #endif
    
    #if TRACE_MEMORY_ACCESSES
    setup.memRead       = (STool_TMemH)MemoryRead;
    setup.memWrite      = (STool_TMemH)MemoryWrite;
    #endif
    
    #if LOGTIME
    #if EXTERNAL_TIMER
    int ds=shmget(25437, sizeof(UINT64), 0600); // ID 25437 for shared memory
	ptimer=(UINT64*)shmat(ds, NULL, SHM_R);
    #else
    // timer thread
    if (PIN_SpawnInternalThread(timerThread, NULL, DEFAULT_THREAD_STACK_SIZE, NULL) == INVALID_THREADID) {
        printf("Can't spawn internal thread! Exiting...");
        exit(EXIT_FAILURE);
    }
    #endif
    #endif
    
    // run application and analysis tool
    STool_Run(&setup);

    return 0;
}

