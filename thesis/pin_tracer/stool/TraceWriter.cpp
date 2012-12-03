// ===================================================================
// TraceWriter functions support the creation of a binary output file
// recording a stream of events. The file has the following format:
//      magic number (4 bytes)
//      number of routine enter events (8 bytes)
//      number of routine exit events (8 bytes)
//      number of timer ticks (8 bytes)
//      #ticks since application started (8 bytes)
//      timer granularity in microseconds (4 bytes)
//      stream of events (rtn enter, rtn exit, timer tic)
// ====================================================================

// Author:       Irene Finocchi
// Last changed: $Date: 2010/11/09 10:16:47 $
// Changed by:   $Author: pctips $
// Revision:     $Revision: 1.18 $

#include "TraceWriter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

// -------------------------------------------------------------
// Constants and types
// -------------------------------------------------------------

#define MAGIC_NUMBER    0xABADCAFE
#define BLOCK_SIZE      BUFSIZ      // from stdio.h
//#define BLOCK_SIZE      8192      // TODO: setvbuf etc etc
#define OPTIMIZE_IO     1

struct TraceWriter {

    // Output file
    FILE* out;
    
    // Internals
    UINT32 bufferSize, used[2]; // up to 4 GB
    void *buffer[2]; // myWriter uses buffer[0]
    int index;
    PIN_THREAD_UID dumpThread;
    
    BOOL logTime;
    UINT32 timerGranularity;
    UINT64 firstTick, ticks, *ptimerTicks;
    
    // Stream stats
    unsigned long long rtnEnterEvents;
    unsigned long long rtnExitEvents;
    
    #if TRACE_MEMORY_ACCESSES
    unsigned long long memReadEvents;
    unsigned long long memWriteEvents;
    #endif
    
    #if DUMP_NAMES
    // Auxiliary data structures
    GHashTable *routine_hash_table;
    GHashTable *library_hash_table;
    #endif
};

enum EVENT_TYPE {
    RTN_ENTER       = 0x0,
    RTN_EXIT        = 0x1, 
    MEM_READ        = 0x2,
    MEM_WRITE       = 0x3,
    TIMER_TICK      = 0x4,
    RTN_ENTER_CS    = 0x5
};

// -------------------------------------------------------------
// TraceWriter buffering and timer schemas
// ------------------------------------------------------------
#if OPTIMIZE_IO
static VOID dumpToFile(VOID *arg) {
    TraceWriter *tr=(TraceWriter*)arg;
    //~ int index=0;
    //~ if (tr->index==0) index=1;
    int index=(tr->index-1)%2; // incremented before PIN_SpawnInternalThread()
    char* buffer=(char*)tr->buffer[index];
    
    #if 1
    if (fwrite(buffer, tr->used[index], 1, tr->out)!=1) {
        printf("Error %d writing to disk! Exiting...\n", errno);
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
                printf("Error %d writing to disk! Exiting...\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        tr->used[index]-=ret;
        buffer+=ret;
    }
    #endif
    
    PIN_ExitThread(0);
}

static VOID mySync(TraceWriter* tr) {
    if (tr->index > 0)
        PIN_WaitForThreadTermination(tr->dumpThread, PIN_INFINITE_TIMEOUT, NULL);
    
    // Synchronous I/O
    int index=tr->index%2; // buffer currently in use
    char* buffer=(char*)tr->buffer[index];
    
    #if 1
    if (fwrite(buffer, tr->used[index], 1, tr->out)!=1) {
        printf("Error %d writing to disk! Exiting...\n", errno);
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
                printf("Error %d writing to disk! Exiting...\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        tr->used[index]-=ret;
        buffer+=ret;
    }
    #endif
}

void myWrite(const void *ptr, size_t size, size_t nitems, TraceWriter *tr) {
    int sizeTot=size*nitems;
    if (sizeTot + tr->used[tr->index%2] > tr->bufferSize) {
        if (tr->index>0)
            PIN_WaitForThreadTermination(tr->dumpThread, PIN_INFINITE_TIMEOUT, NULL);
        tr->index++;
        if (PIN_SpawnInternalThread(dumpToFile, (void*)tr, DEFAULT_THREAD_STACK_SIZE, &(tr->dumpThread)) == INVALID_THREADID) {
            printf("Can't spawn internal thread! Exiting...");
            exit(EXIT_FAILURE);
        }
    }
    memcpy((char*)tr->buffer[tr->index%2]+tr->used[tr->index%2], ptr, sizeTot);
    tr->used[tr->index%2]+=sizeTot;
}
#else
#define myWrite(ptr, size, nitems, tr) fwrite(ptr,size,nitems,tr->out)
#endif

#define checkTimer(tr) if (tr->logTime && tr->ticks!=*(tr->ptimerTicks)) { \
        typeByte = TIMER_TICK; \
        for (int i=*(tr->ptimerTicks)-tr->ticks; i>0; --i, tr->ticks++) \
            myWrite(&typeByte, sizeof(char), 1, tr); \
        }

// -------------------------------------------------------------
// TraceWriter creation function
// -------------------------------------------------------------
TraceWriter* TraceWriter_Open(const char* traceFileName, UINT64 *ptimer, UINT32 timerGranularity, UINT32 bufferSize){

    TraceWriter *tr = (TraceWriter *) malloc(sizeof(TraceWriter));
    if (tr == NULL) return NULL;
    
    int ds = open64(traceFileName, O_EXCL|O_CREAT|O_RDWR, 0660);
    if (ds == -1) {
        char suffix[12], newName[75]; // 64+12-1
        srand(time(NULL));
        sprintf(suffix, ".%d", rand()); // 32 bit => up to 10 digits
        strcpy(newName, traceFileName);
        strcat(newName, suffix);
        ds = open64(newName, O_EXCL|O_CREAT|O_RDWR, 0660);
        if (ds==-1) {
            free(tr);
            return NULL;
        }
        
        printf("WARNING: Log file %s already exists, using %s instead\n",
                traceFileName, newName);
        
        close(ds);
        tr->out=fopen64(newName, "wb+"); // fdopen64 needed :)
    } else {
        close(ds);
        tr->out=fopen64(traceFileName, "wb+"); // fdopen64 needed :)
    }
    
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
    
    // Internal timer
    if (ptimer==NULL) {
        tr->logTime=FALSE;
        // Inizialization needed for dump
        tr->ticks=0;
        tr->firstTick=0;
        tr->timerGranularity=0;
    }
    else {
        tr->logTime=TRUE;
        tr->ptimerTicks=ptimer;
        tr->ticks=*ptimer;
        tr->firstTick=tr->ticks;
        tr->timerGranularity=timerGranularity;
    }
    
    // write magic number
    unsigned long magic = MAGIC_NUMBER;
    myWrite(&magic, sizeof(unsigned long), 1, tr);

    // skip space for file info
    #if DUMP_NAMES
    unsigned long long noValue = 0;
    for (int i=0; i<6; i++)
        myWrite(&noValue, sizeof(unsigned long long), 1, tr);
    #else
    unsigned long noValue = 0;
    for (int i=0; i<9; i++) // 36 bytes (4 UINT64 + 1 UINT32)
        myWrite(&noValue, sizeof(unsigned long), 1, tr);
    #endif    
    
    // init event counters
    tr->rtnEnterEvents = 0;
    tr->rtnExitEvents  = 0;
    
    #if TRACE_MEMORY_ACCESSES
    tr->memReadEvents  = tr->memWriteEvents = 0;
    #endif
    
    #if DUMP_NAMES
    // create auxiliary hash tables    
    tr->routine_hash_table = g_hash_table_new(g_int_hash, g_int_equal);    
    tr->library_hash_table = g_hash_table_new(g_int_hash, g_int_equal);
    #endif
    
    return tr;
}

// -------------------------------------------------------------
// TraceWriter delete function
// -------------------------------------------------------------
BOOL TraceWriter_Close(TraceWriter* tr){
    #if OPTIMIZE_IO
    mySync(tr);
    free(tr->buffer[0]);
    free(tr->buffer[1]);
    #endif
    
    #if DUMP_NAMES==0
    
    unsigned long long tmp=0;
    if ( fseeko(tr->out, (off_t) sizeof(unsigned long), SEEK_SET) == -1 ) return FALSE;
    // Write 4 unsigned long long fields just after the magic number
    fwrite(&tr->rtnEnterEvents, sizeof(unsigned long long), 1, tr->out); 
    fwrite(&tr->rtnExitEvents, sizeof(unsigned long long), 1, tr->out);
    tmp=tr->ticks - tr->firstTick;
    fwrite(&tmp, sizeof(unsigned long long), 1, tr->out);
    fwrite(&tr->firstTick, sizeof(unsigned long long), 1, tr->out);
    fwrite(&tr->timerGranularity, sizeof(unsigned long), 1, tr->out);
    fclose(tr->out);
    free(tr);
    return TRUE;

    #else
    // append routine table at the end of file
    off_t rtnOffset = ftello(tr->out);
    
    guint numRoutines = g_hash_table_size(tr->routine_hash_table);
    fwrite(&numRoutines, sizeof(guint), 1, tr->out);
            
	gpointer key, value;	
	
	GHashTableIter routine_hash_table_iter;
	g_hash_table_iter_init(&routine_hash_table_iter, tr->routine_hash_table);
	
	while (g_hash_table_iter_next(&routine_hash_table_iter, &key, &value)) {

        ADDRINT addr = (ADDRINT) key;
        fwrite(&addr, sizeof(ADDRINT), 1, tr->out);

        const char* rtnName = STool_RoutineNameByAddr(addr);
        
        size_t len = strlen(rtnName);
        fwrite(&len, sizeof(size_t), 1, tr->out);

        fwrite(rtnName, len*sizeof(char), 1, tr->out);
        
        UINT32 libID = STool_LibraryIDByAddr(addr);
        fwrite(&libID, sizeof(UINT32), 1, tr->out);
   	}

    // append library table at the end of file

    off_t libOffset = ftello(tr->out);
    
    guint numLibraries = g_hash_table_size(tr->library_hash_table);
    fwrite(&numLibraries, sizeof(guint), 1, tr->out);
            
	GHashTableIter library_hash_table_iter;
	g_hash_table_iter_init(&library_hash_table_iter, tr->library_hash_table);
	
	while (g_hash_table_iter_next(&library_hash_table_iter, &key, &value)) {

        UINT32 libID = (UINT32) key;
        fwrite(&libID, sizeof(UINT32), 1, tr->out);

        const char* libName = STool_LibraryNameByID(libID);

        size_t len = strlen(libName);
        fwrite(&len, sizeof(size_t), 1, tr->out);

        fwrite(libName, sizeof(char), len, tr->out);
   	}

    // write header
   	if ( fseeko(tr->out, (off_t) sizeof(unsigned long), SEEK_SET) == -1 ) return FALSE;

    fwrite(&rtnOffset, sizeof(off_t), 1, tr->out);
    fwrite(&libOffset, sizeof(off_t), 1, tr->out);   

    fwrite(&(tr->rtnEnterEvents), sizeof(unsigned long long), 1, tr->out);
    fwrite(&(tr->rtnExitEvents),  sizeof(unsigned long long), 1, tr->out);
    fwrite(&(tr->memReadEvents),  sizeof(unsigned long long), 1, tr->out);
    fwrite(&(tr->memWriteEvents), sizeof(unsigned long long), 1, tr->out);
             
    // close output file
    if (fclose(tr->out) == EOF) return FALSE;
    
    // dispose auxiliary hash tables
    g_hash_table_destroy(tr->routine_hash_table);    
    g_hash_table_destroy(tr->library_hash_table);
    
    free(tr);

    return TRUE;
    #endif
}

// -------------------------------------------------------------
// Save routine enter record
// -------------------------------------------------------------
#if CALL_SITE

#if DUMP_NAMES
void TraceWriter_RtnEnter(TraceWriter* tr, 
			              ADDRINT addr, ADDRINT ip,
			              const char* rtnName, 
			              const char* libName) {
#else
void TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr, ADDRINT ip) {
#endif

#else

#if DUMP_NAMES
void TraceWriter_RtnEnter(TraceWriter* tr, 
			              ADDRINT addr, 
			              const char* rtnName, 
			              const char* libName) {
#else
void TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr) {
#endif

#endif
                    
    char typeByte;
    tr->rtnEnterEvents++;
    
    checkTimer(tr);
    
    #if CALL_SITE
    typeByte = RTN_ENTER_CS;
    myWrite(&typeByte, sizeof(char), 1, tr);        
    // 4 bytes for routine address
    myWrite(&addr, sizeof(ADDRINT), 1, tr);
    // 2 bytes for call site (16 LSBs)
    myWrite(&ip, sizeof(ADDRINT)/2, 1, tr);
    #else
    typeByte = RTN_ENTER;
    myWrite(&typeByte, sizeof(char), 1, tr);        
    // 4 bytes for routine address
    myWrite(&addr, sizeof(ADDRINT), 1, tr);
    #endif
    
    #if DUMP_NAMES
    // check if routine/library have already been encountered
    // if not, update the routine/libray hash table
    gboolean isRtnPresent = g_hash_table_lookup_extended(tr->routine_hash_table, (void*) addr, NULL, NULL);
    if (isRtnPresent==FALSE) {
        g_hash_table_insert(tr->routine_hash_table, (void*)addr, NULL);
        UINT32 libID = STool_LibraryIDByAddr(addr);
        gboolean isLibPresent = g_hash_table_lookup_extended(tr->library_hash_table, (void*) &libID, NULL, NULL);
        if (isLibPresent==FALSE) g_hash_table_insert(tr->library_hash_table, (void*) &libID, NULL);
    }
    #endif
}

// -------------------------------------------------------------
// Save routine exit record
// -------------------------------------------------------------
void TraceWriter_RtnExit (TraceWriter* tr) {
    char typeByte;
    tr->rtnExitEvents++;
    
    checkTimer(tr);
    
    typeByte = RTN_EXIT;
    myWrite(&typeByte, sizeof(char), 1, tr);
}

#if TRACE_MEMORY_ACCESSES
// -------------------------------------------------------------
// Save memory read record
// -------------------------------------------------------------
void TraceWriter_MemRead (TraceWriter* tr, 
			              ADDRINT addr) {
    char typeByte;
    tr->memReadEvents++;
    
    checkTimer(tr);

    typeByte = MEM_READ;
    myWrite(&typeByte, sizeof(char), 1, tr);

    // 4 bytes for memory address
    myWrite(&addr, sizeof(ADDRINT), 1, tr);
}

// -------------------------------------------------------------
// Save memory write record
// -------------------------------------------------------------
void TraceWriter_MemWrite(TraceWriter* tr, 
			              ADDRINT addr) {

    char typeByte;
    tr->memWriteEvents++;
    
    checkTimer(tr);

    typeByte = MEM_WRITE;    
    myWrite(&typeByte, sizeof(char), 1, tr);
        
    // 4 bytes for memory address
    myWrite(&addr, sizeof(ADDRINT), 1, tr);
}
#endif
