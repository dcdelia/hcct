// ===============================================================
// TraceWriter functions support the creation of a binary output 
// file recording a stream of events
// ===============================================================

// Author:       Irene Finocchi
// Last changed: $Date: 2010/11/08 23:20:09 $
// Changed by:   $Author: pctips $
// Revision:     $Revision: 1.9 $

#ifndef TRACE_WRITER_H
#define TRACE_WRITER_H

#include "STool.h"

// Both still to be completed
#define DUMP_NAMES				0
#define TRACE_MEMORY_ACCESSES	0

// Calling sites
#ifndef CALL_SITE
#define CALL_SITE               1   // if 1, treat calls from different call sites as different
#endif


typedef struct TraceWriter TraceWriter;

TraceWriter* TraceWriter_Open    (const char* traceFileName, UINT64 *ptimer, UINT32 timerGranularity, UINT32 bufferSize);
BOOL         TraceWriter_Close   (TraceWriter* tr);

#if CALL_SITE

#if DUMP_NAMES
void         TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr, ADDRINT ip, const char* rtnName, const char* libName);
#else
void		 TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr, ADDRINT ip);
#endif

#else

#if DUMP_NAMES
void         TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr, const char* rtnName, const char* libName);
#else
void		 TraceWriter_RtnEnter(TraceWriter* tr, ADDRINT addr);
#endif

#endif

void         TraceWriter_RtnExit (TraceWriter* tr);

#if TRACE_MEMORY_ACCESSES
void         TraceWriter_MemRead (TraceWriter* tr, ADDRINT addr);
void         TraceWriter_MemWrite(TraceWriter* tr, ADDRINT addr);
#endif

#endif

