// ===================================================================
// TraceReader functions support the generation of a stream of events
// read from a binary input file recorded by TraceWriter. 
// The file is assumed to have the following format:
//      magic number (4 bytes)
//      offset of symbol table (8 bytes)
//      offset of library table (8 bytes)
//      number of routine enter events (8 bytes)
//      number of routine exit events (8 bytes)
//      number of memory read events (8 bytes)
//      number of memory write events (8 bytes)
//      stream of events (rtn enter, rtn exit, mem read, meme write)
//      number of routines (4 bytes)
//      routine table (rtn address, strlen(rtn name), rtn name, lib ID)
//      number of libraries (4 bytes)
//      library table (lib ID, strlen(library name), library name)
// ====================================================================

// Author:       Irene Finocchi
// Last changed: $Date: 2010/05/03 17:05:42 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.3 $

#include "TraceReader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

// -------------------------------------------------------------
// Constants and types
// -------------------------------------------------------------

#define MAGIC_NUMBER  0xABADCAFE

struct TraceReader {

    // Output file
    FILE* in;
    
    // Time
    UINT64 time;
    
    // Stream stats
    unsigned long long rtnEnterEvents;
    unsigned long long rtnExitEvents;
    unsigned long long memReadEvents;
    unsigned long long memWriteEvents;
    unsigned long long N;
    
    // Tables' offsets
    off_t rtnTableOffset;
    off_t libTableOffset;
    
    // Auxiliary data structures
    GHashTable *routine_hash_table;
    GHashTable *library_hash_table;
};

enum EVENT_TYPE {
    RTN_ENTER = 0x0,
    RTN_EXIT  = 0x1,
    MEM_READ  = 0x2,
    MEM_WRITE = 0x3
};

// -------------------------------------------------------------
// TraceReader creation function
// -------------------------------------------------------------
TraceReader* TraceReader_Open(const char* traceFileName){

    TraceReader *tr = (TraceReader *) malloc(sizeof(TraceReader));
    if (tr == NULL) return NULL;

    // open input file
    tr->in = fopen(traceFileName, "rb");
    if (tr->in == NULL) {
        free(tr);
	    return NULL;
    }

    // check magic number
    unsigned long magic;
    fread(&magic, sizeof(unsigned long), 1, tr->in);
    if (magic != MAGIC_NUMBER) {
        free(tr);
	    return NULL;
    }

    // init table offsets
    fread(&(tr->rtnTableOffset), sizeof(off_t), 1, tr->in);   
    fread(&(tr->libTableOffset), sizeof(off_t), 1, tr->in);       
    
    // init event counters
    fread(&(tr->rtnEnterEvents), sizeof(unsigned long long), 1, tr->in);   
    fread(&(tr->rtnExitEvents ), sizeof(unsigned long long), 1, tr->in);   
    fread(&(tr->memReadEvents ), sizeof(unsigned long long), 1, tr->in);   
    fread(&(tr->memWriteEvents), sizeof(unsigned long long), 1, tr->in);
    
    // init stream size    
    tr->N = tr->rtnEnterEvents + tr->rtnExitEvents + tr->memReadEvents + tr->memWriteEvents;
    
    // create auxiliary hash tables    
    tr->routine_hash_table = g_hash_table_new(g_int_hash, g_int_equal);    
    tr->library_hash_table = g_hash_table_new(g_int_hash, g_int_equal);
    
    return tr;
}

// -------------------------------------------------------------
// TraceReader delete function
// -------------------------------------------------------------
BOOL TraceReader_Close(TraceReader* tr){

    // append routine table at the end of file
    
    off_t rtnOffset = ftello(tr->in);
    
    guint numRoutines = g_hash_table_size(tr->routine_hash_table);
    fwrite(&numRoutines, sizeof(guint), 1, tr->in);
            
	gpointer key, value;	
	
	GHashTableIter routine_hash_table_iter;
	g_hash_table_iter_init(&routine_hash_table_iter, tr->routine_hash_table);
	
	while (g_hash_table_iter_next(&routine_hash_table_iter, &key, &value)) {

        ADDRINT addr = (ADDRINT) key;
        fwrite(&addr, sizeof(ADDRINT), 1, tr->in);

        const char* rtnName = STool_RoutineNameByAddr(addr);
        
        size_t len = strlen(rtnName);
        fwrite(&len, sizeof(size_t), 1, tr->in);

        fwrite(rtnName, sizeof(char), len, tr->in);
        
        UINT32 libID = STool_LibraryIDByAddr(addr);
        fwrite(&libID, sizeof(UINT32), 1, tr->in);
   	}

    // append library table at the end of file

    off_t libOffset = ftello(tr->in);
    
    guint numLibraries = g_hash_table_size(tr->library_hash_table);
    fwrite(&numLibraries, sizeof(guint), 1, tr->in);
            
	GHashTableIter library_hash_table_iter;
	g_hash_table_iter_init(&library_hash_table_iter, tr->library_hash_table);
	
	while (g_hash_table_iter_next(&library_hash_table_iter, &key, &value)) {

        UINT32 libID = (UINT32) key;
        fwrite(&libID, sizeof(UINT32), 1, tr->in);

        const char* libName = STool_LibraryNameByID(libID);
        
        size_t len = strlen(libName);
        fwrite(&len, sizeof(size_t), 1, tr->in);

        fwrite(libName, sizeof(char), len, tr->in);
   	}
   	
    // write header
   	if ( fseeko(tr->in, (off_t) sizeof(unsigned long), SEEK_SET) == -1 ) return FALSE;
   	
    fwrite(&rtnOffset, sizeof(off_t), 1, tr->in);
    fwrite(&libOffset, sizeof(off_t), 1, tr->in);   

    fwrite(&(tr->rtnEnterEvents), sizeof(unsigned long long), 1, tr->in);
    fwrite(&(tr->rtnExitEvents),  sizeof(unsigned long long), 1, tr->in);
    fwrite(&(tr->memReadEvents),  sizeof(unsigned long long), 1, tr->in);
    fwrite(&(tr->memWriteEvents), sizeof(unsigned long long), 1, tr->in);
                
    // close output file
    if (fclose(tr->in) == EOF) return FALSE;
    
    // dispose auxiliary hash tables
    g_hash_table_destroy(tr->routine_hash_table);    
    g_hash_table_destroy(tr->library_hash_table);
        
    free(tr);
    return TRUE;
}

// -------------------------------------------------------------
// Save routine enter record
// -------------------------------------------------------------
void TraceReader_RtnEnter(TraceReader* tr, 
			              ADDRINT addr, 
			              const char* rtnName, 
			              const char* libName, 
			              BOOL logTime, 
			              UINT64 time) {

    tr->rtnEnterEvents++;
    
    char typeByte = 0;
    
    if (logTime == TRUE) typeByte |= 0x80;
    typeByte |= RTN_ENTER;

    // 1 byte for event type and time logging flag
    fwrite(&typeByte, sizeof(char), 1, tr->in);
        
    // 4 bytes for timestamp
    if (logTime == TRUE) fwrite(&time, sizeof(UINT64), 1, tr->in);

    // 4 bytes for routine address
    fwrite(&addr, sizeof(ADDRINT), 1, tr->in);
    
    // check if routine/library have already been encountered
    // if not, update the routine/libray hash table
    gboolean isRtnPresent = g_hash_table_lookup_extended(tr->routine_hash_table, (void*) addr, NULL, NULL);
    
    if (isRtnPresent==FALSE) {
        g_hash_table_insert(tr->routine_hash_table, (void*)addr, NULL);
                
        gboolean isLibPresent = g_hash_table_lookup_extended(tr->library_hash_table, (void*) STool_LibraryIDByAddr(addr), NULL, NULL);
        
        if (isLibPresent==FALSE) g_hash_table_insert(tr->library_hash_table, (void*) STool_LibraryIDByAddr(addr), NULL);
    }
}


// -------------------------------------------------------------
// Save routine exit record
// -------------------------------------------------------------
void TraceReader_RtnExit (TraceReader* tr, 
			              BOOL logTime, 
			              UINT64 time) {

    tr->rtnExitEvents++;
    
    char typeByte = 0;
    
    if (logTime == TRUE) typeByte |= 0x80;
    typeByte |= RTN_EXIT;

    // 1 byte for event type and time logging flag
    fwrite(&typeByte, sizeof(char), 1, tr->in);
        
    // 4 bytes for timestamp
    if (logTime == TRUE) fwrite(&time, sizeof(UINT64), 1, tr->in);
}

// -------------------------------------------------------------
// Save memory read record
// -------------------------------------------------------------
void TraceReader_MemRead (TraceReader* tr, 
			              ADDRINT addr, 
			              BOOL logTime, 
			              UINT64 time) {

    tr->memReadEvents++;
    
    char typeByte = 0;
    
    //if (logTime == TRUE) typeByte |= 0x80;
    typeByte |= MEM_READ;

    // 1 byte for event type and time logging flag
    fwrite(&typeByte, sizeof(char), 1, tr->in);
        
    // 4 bytes for timestamp
    //if (logTime == TRUE) fwrite(&time, sizeof(UINT64), 1, tr->in);

    // 4 bytes for memory address
    fwrite(&addr, sizeof(ADDRINT), 1, tr->in);
}

// -------------------------------------------------------------
// Save memory write record
// -------------------------------------------------------------
void TraceReader_MemWrite(TraceReader* tr, 
			              ADDRINT addr, 
			              BOOL logTime, 
			              UINT64 time) {

    tr->memWriteEvents++;
    
    char typeByte = 0;
    
    //if (logTime == TRUE) typeByte |= 0x80;
    typeByte |= MEM_WRITE;

    // 1 byte for event type and time logging flag
    fwrite(&typeByte, sizeof(char), 1, tr->in);
        
    // 4 bytes for timestamp
    //if (logTime == TRUE) fwrite(&time, sizeof(UINT64), 1, tr->in);

    // 4 bytes for memory address
    fwrite(&addr, sizeof(ADDRINT), 1, tr->in);
}

