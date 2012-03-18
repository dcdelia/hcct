// ====================================================================
// TraceReader functions support the generation of a stream of events
// read from a binary input file recorded by TraceWriter. 
// ====================================================================

// Author:       Irene Finocchi
// Last changed: $Date: 2010/05/03 13:28:41 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.2 $

#ifndef TRACE_READER_H
#define TRACE_READER_H

#include "STool.h"

typedef struct TraceReader TraceReader;

TraceReader* TraceReader_Open    (const char* traceFileName);
BOOL         TraceReader_Close   (TraceReader* tr);

void         TraceReader_ScanEvents(TraceReader* tr, 
                                    void (*rtnEnterHandler)(ADDRINT addr),
                                    void (*rtnExitHandler )(ADDRINT addr),                                    
                                    void (*memReadHandler )(ADDRINT addr),
                                    void (*memWriteHandler)(ADDRINT addr)
                                   );

void         TraceReader_RtnEnter(TraceReader* tr, ADDRINT addr, const char* rtnName, const char* libName, BOOL logTime, UINT64 time);
void         TraceReader_RtnExit (TraceReader* tr, BOOL logTime, UINT64 time);
void         TraceReader_MemRead (TraceReader* tr, ADDRINT addr, BOOL logTime, UINT64 time);
void         TraceReader_MemWrite(TraceReader* tr, ADDRINT addr, BOOL logTime, UINT64 time);

#endif

