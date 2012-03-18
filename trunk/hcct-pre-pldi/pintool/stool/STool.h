// =============================================================
// Generic Instrumentation and Trace Replay API
// =============================================================

// Author:       Camil Demetrescu
// Last changed: $Date: 2010/10/22 21:29:01 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.13 $

#ifndef STREAMTOOL_H
#define STREAMTOOL_H

// Supported analysis toolkits
#ifndef PIN_BASED
#define PIN_BASED 1
#endif
#define TRACE_BASED 2

// Defined stream generation toolkit
#ifndef INS_TOOLKIT
#define INS_TOOLKIT PIN_BASED     // use TRACE to compile without PIN (trace replay)
#endif

#if INS_TOOLKIT == PIN_BASED
#include "pin.H"

#else

typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef signed char         INT8;
typedef signed short        INT16;
typedef signed int          INT32;
typedef signed long long    INT64;
typedef UINT32              ADDRINT;
typedef UINT32              THREADID;
typedef UINT8               BOOL;

#ifndef TRUE
#define TRUE                1
#endif

#ifndef FALSE
#define FALSE               0
#endif

#endif

// Constants
#define STool_INVALID_LIB   ((UINT32)-1)
#define _STOOL_PAD_SIZE     64          // struct padding size (avoiding false sharing performance degrading)

// Internal macros
#define _STool_ThreadRec(t) ((_STool_TThreadRec*)(t)-1)

// Memory operation record
typedef struct {
	ADDRINT			addr;		   // address of memory operation
	BOOL 			isWrite;	   // whether this operation is a memory read or write
} STool_TMemRec;

// Callback types
typedef void (*STool_TFiniH)    ();
typedef void (*STool_TThreadH)  (void* threadRec);
typedef void (*STool_TRtnH)     (void* threadRec, ...);
typedef void (*STool_TMemH)     (void* threadRec, ADDRINT addr);
typedef void (*STool_TBufH)		(void* threadRec, STool_TMemRec* buf, UINT64 numRec);

// Analysis tool setup record
typedef struct {
    int             argc;
    char**          argv;
    size_t          activationRecSize;  // user-defined activation record
    size_t          threadRecSize;      // user-defined thread local data record
    BOOL            callingSite;        // if TRUE, pass to rtnEnter as second parameter the address of the calling instruction
    STool_TFiniH    appEnd;             // application termination event callback
    STool_TThreadH  threadStart;        // thread start event callback
    STool_TThreadH  threadEnd;          // thread end event callback
    STool_TRtnH     rtnEnter;           // routine enter event callback
    STool_TRtnH     rtnExit;            // routine exit event callback
    STool_TMemH     memRead;            // memory read operation event callback
    STool_TMemH     memWrite;           // memory write operation event callback
    STool_TBufH		memBuf;				// trace buffer processing function. If not null, memRead and memWrite are ignored
} STool_TSetup;

// Internal stack activation record
typedef struct {
    ADDRINT         target;        // address of activated function
    ADDRINT         currentSP;     // stack pointer register content before the call    
} _STool_TActivationRec;

// Internal thread data record
typedef struct {
    _STool_TActivationRec* activationStack;
    char*                  userStack;
    UINT32                 stackTop;
    UINT32                 stackMaxSize;
    THREADID               threadID;
    void*				   buf_addr;			   // trace buffer start address. Not null when using memory operations buffering
    UINT8                  pad[_STOOL_PAD_SIZE];   // to avoid false sharing problems    
} _STool_TThreadRec;

// Public functions
void            STool_Run(STool_TSetup* env);
const char*     STool_AppName();
const char*     STool_CommandLine();
const char*     STool_RoutineNameByAddr(ADDRINT rtnAddr);
const char*     STool_RoutineDemangledNameByAddr(ADDRINT rtnAddr, BOOL full);
const char*     STool_LibraryNameByAddr(ADDRINT rtnAddr);
UINT32          STool_LibraryIDByAddr(ADDRINT rtnAddr);
const char*     STool_LibraryNameByID(UINT32 libID);
const char*     STool_StripPath(const char* pathName);

// Public macros
#define STool_ThreadID(t)          (_STool_ThreadRec(t)->threadID)
#define STool_NumActivations(t)    (_STool_ThreadRec(t)->stackTop)
#define STool_ActivationAt(t,i,T)  (*(T*)(_STool_ThreadRec(t)->userStack+(i)*sizeof(T)))
#define STool_CurrActivation(t,T)  STool_ActivationAt(t,STool_NumActivations(t)-1,T)
#define STool_RoutineAt(t,i)       (_STool_ThreadRec(t)->activationStack[i].target)
#define STool_CurrRoutine(t)       STool_RoutineAt(t, STool_NumActivations(t)-1)
#define STool_StackPointerAt(t,i)  (_STool_ThreadRec(t)->activationStack[i].currentSP)
#define STool_CurrStackPointer(t)  STool_StackPointerAt(t, STool_NumActivations(t)-1)

#endif

