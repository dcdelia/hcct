// =============================================================
// Generic Instrumentation API based on Intel PIN
// =============================================================

// Author:       Camil Demetrescu
// Last changed: $Date: 2011/09/24 08:53:18 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.17 $

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include "STool.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// -------------------------------------------------------------
// Constants
// -------------------------------------------------------------

#define PAD_SIZE    64          // struct padding size (avoiding false sharing performance degrading)
#define INIT_STACK  64          // initial stack size
#define BUF_SIZE	2			// trace buffer size in pages
#define DEBUG       0           // if 1, trace analysis operations on stdout
#define DEBUG_INS   0           // if 1, trace instrumentation operations on stdout


// -------------------------------------------------------------
// Utility macros
// -------------------------------------------------------------

#define _ActivationAt(t,i) ((t)->activationStack[i])


// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

static char*         gAppName = (char*)"?";   // application name
static char*         gCommandLine = NULL;     // command line
static TLS_KEY       gTlsKey;                 // key for accessing TLS storage in the threads. initialized once in main()
static BUFFER_ID	 buf_id;				  // key for performing trace buffer management operations
static STool_TSetup  gSetup;                  // global pin tool configuration


// -------------------------------------------------------------
// Prototypes
// -------------------------------------------------------------
VOID ThreadEnd(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v);


// =============================================================
// Utility functions
// =============================================================

// -------------------------------------------------------------
// Print help message                                                
// -------------------------------------------------------------
INT32 Usage() {
    fprintf(stderr, "%s\n", KNOB_BASE::StringKnobSummary().c_str());
    return -1;
}


// -------------------------------------------------------------
// Abort application and gracefully end thread
// -------------------------------------------------------------
VOID graceful_exit(THREADID threadid, const char* msg) {
    ThreadEnd(threadid, 0, 0, 0);
    exit((printf("[TID=%u] %s\n", threadid, msg), 1));
}


// -------------------------------------------------------------
// Abort application
// -------------------------------------------------------------
VOID panic(THREADID threadid, const char* msg) {
    exit((printf("[TID=%u] %s\n", threadid, msg), 1));
}


// -------------------------------------------------------------
// Get function address by branch target address
// -------------------------------------------------------------
ADDRINT Target2FunAddr(ADDRINT target) {
    PIN_LockClient();
    
    ADDRINT funAddr;
    
    const RTN rtn = RTN_FindByAddress(target);

    if ( RTN_Valid(rtn) ) 
         funAddr = RTN_Address(rtn);
    else funAddr = target;

    PIN_UnlockClient();

    return funAddr;
}


// -------------------------------------------------------------
// Check if routine is plt stub
// -------------------------------------------------------------
static BOOL IsPLT(RTN rtn) {

    // All .plt thunks have a valid RTN
    if (!RTN_Valid(rtn)) return FALSE;

    if (".plt" == SEC_Name(RTN_Sec(rtn))) return TRUE;
    return FALSE;
}


// -------------------------------------------------------------
// Access thread-specific data
// -------------------------------------------------------------
static inline _STool_TThreadRec* getTLS(THREADID threadid) {
    _STool_TThreadRec* tdata = static_cast<_STool_TThreadRec*>(PIN_GetThreadData(gTlsKey, threadid));
    return tdata;
}



// =============================================================
// Callback functions
// =============================================================

// -------------------------------------------------------------
// Thread start function
// -------------------------------------------------------------
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {

    // create local thread data
    _STool_TThreadRec* tdata = (_STool_TThreadRec*)malloc(sizeof(_STool_TThreadRec) + gSetup.threadRecSize);
    if (tdata == NULL) panic(threadid, "Can't allocate thread specific info");

    // init local thread data
    tdata->threadID     = threadid;
    tdata->stackTop     = 0;
    tdata->stackMaxSize = INIT_STACK;
    
    // create initial shadow activation stack
    tdata->activationStack     = (_STool_TActivationRec*)malloc(tdata->stackMaxSize*sizeof(_STool_TActivationRec));
    if (tdata->activationStack == NULL) panic(threadid, "Can't allocate shadow activation stack");
    
    // create initial user stack (if any was requested by the analysis tool)
    if (gSetup.activationRecSize > 0) {
        tdata->userStack       = (char*)malloc(tdata->stackMaxSize*gSetup.activationRecSize);
        if (tdata->userStack   == NULL) panic(threadid, "Can't allocate user stack");
    }
    else tdata->userStack = NULL;
    
    // if using memory operations buffering, initialize trace buffer pointer
    if (gSetup.memBuf) tdata->buf_addr = PIN_GetBufferPointer(ctxt, buf_id);

    // call thread start callback (if any was specified by the analysis tool)
    if (gSetup.threadStart)
        gSetup.threadStart(tdata + 1);

    // associate thread-specific data to thread
    PIN_SetThreadData(gTlsKey, tdata, threadid);
}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
VOID ThreadEnd(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v) {

    // get pointer to thread local data
    _STool_TThreadRec* tdata = getTLS(threadid);

    // call thread end callback (if any was specified by the analysis tool)
    if (gSetup.threadEnd)
        gSetup.threadEnd(tdata + 1);

    // free shadow activation stack
    free(tdata->activationStack);

    // free user stack (if any)
    if (tdata->userStack) free(tdata->userStack);

    // free thread local data
    free(tdata);
}


// -------------------------------------------------------------
// Buffer overflow function
// -------------------------------------------------------------
VOID* BufferOverflow(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf, UINT64 numElements, VOID *v) {

	_STool_TThreadRec* tdata = getTLS(tid);
	// call user-defined trace buffer processing function
	gSetup.memBuf(tdata+1, static_cast<STool_TMemRec*>(tdata->buf_addr), numElements - (UINT64)(((ADDRINT)tdata->buf_addr - (ADDRINT)buf) / sizeof(STool_TMemRec)));
	tdata->buf_addr = buf;
	return buf;
}


// =============================================================
// Analysis functions
// =============================================================


// -------------------------------------------------------------
// Roll back shadow stack if we got here from a longjmp
// (Runtime stack grows down and register stack grows up)
// -------------------------------------------------------------
static inline VOID AdjustStack(_STool_TThreadRec* tdata, ADDRINT currentSP) {
  
    // roll back stack
    while ( tdata->stackTop > 0 && 
            currentSP >= _ActivationAt(tdata, tdata->stackTop-1).currentSP ) {

        // call routine exit callback (if any was specified by the analysis tool)
        if (gSetup.rtnExit)
            gSetup.rtnExit(tdata + 1);

        // pop activation
        tdata->stackTop--;

        #if DEBUG
        printf("[TID=%u] <<< longjmp: popping activation of %s\n", 
            tdata->threadID, 
            STool_RoutineNameByAddr(_ActivationAt(tdata, tdata->stackTop).target));
        #endif
    }
}


// -------------------------------------------------------------
// Roll back shadow stack if we got here from a longjmp
// (Runtime stack grows down and register stack grows up)
// -------------------------------------------------------------
static inline VOID AdjustStackBuf(_STool_TThreadRec* tdata, ADDRINT currentSP, CONTEXT *ctxt) {

    // roll back stack
    while ( tdata->stackTop > 0 && 
            currentSP >= _ActivationAt(tdata, tdata->stackTop-1).currentSP ) {
        
        // if using memory operations buffering, call trace buffer processing callback 
    	// and reset buffer
    	if (gSetup.memBuf) {
    		void * curr_buf_pointer = PIN_GetBufferPointer(ctxt, buf_id);
    		UINT64 numElements = ((ADDRINT)curr_buf_pointer - (ADDRINT)tdata->buf_addr) / sizeof(STool_TMemRec);
    		gSetup.memBuf(tdata+1, static_cast<STool_TMemRec*>(tdata->buf_addr), numElements);
    		tdata->buf_addr = curr_buf_pointer;
   	 	}

        // call routine exit callback (if any was specified by the analysis tool)
        if (gSetup.rtnExit)
            gSetup.rtnExit(tdata + 1);

        // pop activation
        tdata->stackTop--;

        #if DEBUG
        printf("[TID=%u] <<< longjmp: popping activation of %s\n", 
            tdata->threadID, 
            Target2RtnName(_ActivationAt(tdata, tdata->stackTop).target).c_str());
        #endif
    }
}


// -------------------------------------------------------------
// Function direct call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessDirectCall(ADDRINT target, ADDRINT sp, THREADID threadid) {

    // get thread local data
    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStack(tdata, sp);

    // possibly expand stack
    if (tdata->stackTop >= tdata->stackMaxSize) {

        // double stack size
        tdata->stackMaxSize <<= 1;

        // expand activation stack
        tdata->activationStack = 
            (_STool_TActivationRec*)realloc(tdata->activationStack, 
                                            tdata->stackMaxSize*sizeof(_STool_TActivationRec)); 
        if (tdata->activationStack == NULL) 
            graceful_exit(threadid, "Can't expand activation stack");

        // expand user stack (if any is needed)
        if (tdata->userStack) {
            tdata->userStack = (char*)realloc(tdata->userStack, 
                                              tdata->stackMaxSize*gSetup.activationRecSize); 
            if (tdata->userStack == NULL) 
                graceful_exit(threadid, "Can't expand user stack");
        }
    }    

    // push current activation record to stack
    _ActivationAt(tdata, tdata->stackTop).currentSP = sp;
    _ActivationAt(tdata, tdata->stackTop).target    = target;

    // increase activations counter
    tdata->stackTop++;

    #if DEBUG
    printf("[TID=%u] Entering %s - # activations = %d - stack size = %d\n", 
        threadid, Target2RtnName(target).c_str(), tdata->stackTop, tdata->stackSize);
    #endif
 
    // call routine enter callback (if any was specified by the analysis tool)
    if (gSetup.rtnEnter)
        gSetup.rtnEnter(tdata + 1);
}


// -------------------------------------------------------------
// Function direct call event (with calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessDirectCallCS(ADDRINT ip, ADDRINT target, ADDRINT sp, THREADID threadid) {

    // get thread local data
    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStack(tdata, sp);

    // possibly expand stack
    if (tdata->stackTop >= tdata->stackMaxSize) {

        // double stack size
        tdata->stackMaxSize <<= 1;

        // expand activation stack
        tdata->activationStack = 
            (_STool_TActivationRec*)realloc(tdata->activationStack, 
                                            tdata->stackMaxSize*sizeof(_STool_TActivationRec)); 
        if (tdata->activationStack == NULL) 
            graceful_exit(threadid, "Can't expand activation stack");

        // expand user stack (if any is needed)
        if (tdata->userStack) {
            tdata->userStack = (char*)realloc(tdata->userStack, 
                                              tdata->stackMaxSize*gSetup.activationRecSize); 
            if (tdata->userStack == NULL) 
                graceful_exit(threadid, "Can't expand user stack");
        }
    }    

    // push current activation record to stack
    _ActivationAt(tdata, tdata->stackTop).currentSP = sp;
    _ActivationAt(tdata, tdata->stackTop).target    = target;

    // increase activations counter
    tdata->stackTop++;

    #if DEBUG
    printf("[TID=%u] Entering %s - # activations = %d - stack size = %d\n", 
        threadid, Target2RtnName(target).c_str(), tdata->stackTop, tdata->stackSize);
    #endif
 
    // call routine enter callback (if any was specified by the analysis tool)
    if (gSetup.rtnEnter)
        gSetup.rtnEnter(tdata + 1, ip);
}


// -------------------------------------------------------------
// Function indirect call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessIndirectCall(ADDRINT target, ADDRINT sp, THREADID threadid) {
    A_ProcessDirectCall(Target2FunAddr(target), sp, threadid);
}


// -------------------------------------------------------------
// Function indirect call event (with calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessIndirectCallCS(ADDRINT ip, ADDRINT target, ADDRINT sp, THREADID threadid) {
    A_ProcessDirectCallCS(ip, Target2FunAddr(target), sp, threadid);
}


// -------------------------------------------------------------
// Function exit event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessReturn(ADDRINT sp, THREADID threadid) {

    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStack(tdata, sp);

    if ( tdata->stackTop < 1 ) 
        graceful_exit(threadid, "Internal error: stack bottomed out");

    // call routine exit callback (if any was specified by the analysis tool)
    if (gSetup.rtnExit)
        gSetup.rtnExit(tdata + 1);

    // pop activation
    tdata->stackTop--;

    #if DEBUG
    printf("[TID=%u] Leaving %s\n", 
        threadid,
        Target2RtnName(tdata->activationStack[tdata->stackTop].target).c_str());
    #endif

    #if DEBUG
    if (tdata->stackTop > 0)
        printf("[TID=%u] Back to %s - # activations = %d - stack size = %d\n", 
            threadid, 
            Target2RtnName(tdata->activationStack[tdata->stackTop-1].target).c_str(), 
            tdata->stackTop, 
            tdata->stackSize);
    else printf("[TID=%u] Back to stack bottom\n", threadid);
    #endif
}


// -------------------------------------------------------------
// Function direct call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessDirectCallBuf(ADDRINT target, ADDRINT sp, THREADID threadid, CONTEXT *ctxt) {

    // get thread local data
    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStackBuf(tdata, sp, ctxt);

    // possibly expand stack
    if (tdata->stackTop >= tdata->stackMaxSize) {

        // double stack size
        tdata->stackMaxSize <<= 1;

        // expand activation stack
        tdata->activationStack = 
            (_STool_TActivationRec*)realloc(tdata->activationStack, 
                                            tdata->stackMaxSize*sizeof(_STool_TActivationRec)); 
        if (tdata->activationStack == NULL) 
            graceful_exit(threadid, "Can't expand activation stack");

        // expand user stack (if any is needed)
        if (tdata->userStack) {
            tdata->userStack = (char*)realloc(tdata->userStack, 
                                              tdata->stackMaxSize*gSetup.activationRecSize); 
            if (tdata->userStack == NULL) 
                graceful_exit(threadid, "Can't expand user stack");
        }
    }    

    // push current activation record to stack
    _ActivationAt(tdata, tdata->stackTop).currentSP = sp;
    _ActivationAt(tdata, tdata->stackTop).target    = target;

    // increase activations counter
    tdata->stackTop++;

    #if DEBUG
    printf("[TID=%u] Entering %s - # activations = %d - stack size = %d\n", 
        threadid, Target2RtnName(target).c_str(), tdata->stackTop, tdata->stackSize);
    #endif
 
    // call routine enter callback (if any was specified by the analysis tool)
    if (gSetup.rtnEnter)
        gSetup.rtnEnter(tdata + 1);
}


// -------------------------------------------------------------
// Function direct call event (with calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessDirectCallCSBuf(ADDRINT ip, ADDRINT target, ADDRINT sp, THREADID threadid, CONTEXT *ctxt) {

    // get thread local data
    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStackBuf(tdata, sp, ctxt);

    // possibly expand stack
    if (tdata->stackTop >= tdata->stackMaxSize) {

        // double stack size
        tdata->stackMaxSize <<= 1;

        // expand activation stack
        tdata->activationStack = 
            (_STool_TActivationRec*)realloc(tdata->activationStack, 
                                            tdata->stackMaxSize*sizeof(_STool_TActivationRec)); 
        if (tdata->activationStack == NULL) 
            graceful_exit(threadid, "Can't expand activation stack");

        // expand user stack (if any is needed)
        if (tdata->userStack) {
            tdata->userStack = (char*)realloc(tdata->userStack, 
                                              tdata->stackMaxSize*gSetup.activationRecSize); 
            if (tdata->userStack == NULL)
                graceful_exit(threadid, "Can't expand user stack");
        }
    }    

    // push current activation record to stack
    _ActivationAt(tdata, tdata->stackTop).currentSP = sp;
    _ActivationAt(tdata, tdata->stackTop).target    = target;

    // increase activations counter
    tdata->stackTop++;

    #if DEBUG
    printf("[TID=%u] Entering %s - # activations = %d - stack size = %d\n", 
        threadid, Target2RtnName(target).c_str(), tdata->stackTop, tdata->stackSize);
    #endif
 
    // call routine enter callback (if any was specified by the analysis tool)
    if (gSetup.rtnEnter)
        gSetup.rtnEnter(tdata + 1, ip);
}


// -------------------------------------------------------------
// Function indirect call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessIndirectCallBuf(ADDRINT target, ADDRINT sp, THREADID threadid, CONTEXT *ctxt) {
    A_ProcessDirectCallBuf(Target2FunAddr(target), sp, threadid, ctxt);
}


// -------------------------------------------------------------
// Function indirect call event (with calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessIndirectCallCSBuf(ADDRINT ip, ADDRINT target, ADDRINT sp, THREADID threadid, CONTEXT *ctxt) {
    A_ProcessDirectCallCSBuf(ip, Target2FunAddr(target), sp, threadid, ctxt);
}


// -------------------------------------------------------------
// Function exit event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessReturnBuf(ADDRINT sp, THREADID threadid, CONTEXT *ctxt) {
    
    _STool_TThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStackBuf(tdata, sp, ctxt);

    if ( tdata->stackTop < 1 )
        graceful_exit(threadid, "Internal error: stack bottomed out");
        
    // if using memory operations buffering, call trace buffer processing callback 
    // and reset buffer
    if (gSetup.memBuf) {
    	void * curr_buf_pointer = PIN_GetBufferPointer(ctxt, buf_id);
    	UINT64 numElements = ((ADDRINT)curr_buf_pointer - (ADDRINT)tdata->buf_addr) / sizeof(STool_TMemRec);
    	gSetup.memBuf(tdata+1, static_cast<STool_TMemRec*>(tdata->buf_addr), numElements);
    	tdata->buf_addr = curr_buf_pointer;
    }
    
    // call routine exit callback (if any was specified by the analysis tool)
    if (gSetup.rtnExit)
        gSetup.rtnExit(tdata + 1);

    // pop activation
    tdata->stackTop--;

    #if DEBUG
    printf("[TID=%u] Leaving %s\n", 
        threadid,
        Target2RtnName(tdata->activationStack[tdata->stackTop].target).c_str());
    #endif

    #if DEBUG
    if (tdata->stackTop > 0)
        printf("[TID=%u] Back to %s - # activations = %d - stack size = %d\n", 
            threadid, 
            Target2RtnName(tdata->activationStack[tdata->stackTop-1].target).c_str(), 
            tdata->stackTop, 
            tdata->stackSize);
    else printf("[TID=%u] Back to stack bottom\n", threadid);
    #endif
}


// -------------------------------------------------------------
// Memory read event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_MemRead(THREADID threadid, ADDRINT addr) {
    gSetup.memRead((_STool_TThreadRec*)getTLS(threadid)+1, addr);
}


// -------------------------------------------------------------
// Memory write event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_MemWrite(THREADID threadid, ADDRINT addr) {
    gSetup.memWrite((_STool_TThreadRec*)getTLS(threadid)+1, addr);
}


// =============================================================
// Instrumentation functions
// =============================================================

// -------------------------------------------------------------
// Instruction instrumentation function
// -------------------------------------------------------------
// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins) {

    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // The IA-64 architecture has explicitly predicated instructions. 
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {

        if (INS_MemoryOperandIsRead(ins, memOp)) {

            #if DEBUG_INS    
            printf("-- Instrumenting read operation at instruction address %X\n", INS_Address(ins));
            #endif

            if (gSetup.memBuf) { // if using memory operations buffering, bypass user-defined callbacks
            	INS_InsertFillBufferPredicated(ins, IPOINT_BEFORE, buf_id,
                                     IARG_MEMORYOP_EA, memOp, offsetof(STool_TMemRec, addr),
                                     IARG_BOOL, FALSE, offsetof(STool_TMemRec, isWrite),
                                     IARG_END);
            }
            else if (gSetup.memRead) {
		        INS_InsertPredicatedCall(
		        	ins, IPOINT_BEFORE, 
		        	(AFUNPTR)A_MemRead,
		        	IARG_FAST_ANALYSIS_CALL,
		        	IARG_THREAD_ID,
		        	IARG_MEMORYOP_EA, memOp,
		        	IARG_END);
            }
        }

        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp)) {

            #if DEBUG_INS    
            printf("-- Instrumenting write operation at instruction address %X\n", INS_Address(ins));
            #endif

            if (gSetup.memBuf) { // if using memory operations buffering, bypass user-defined callbacks
            	INS_InsertFillBufferPredicated(ins, IPOINT_BEFORE, buf_id,
                                     IARG_MEMORYOP_EA, memOp, offsetof(STool_TMemRec, addr),
                                     IARG_BOOL, TRUE, offsetof(STool_TMemRec, isWrite),
                                     IARG_END);
            }
            else if (gSetup.memWrite) {
		        INS_InsertPredicatedCall(
		        	ins, IPOINT_BEFORE, 
		        	(AFUNPTR)A_MemWrite,
		        	IARG_FAST_ANALYSIS_CALL,
		        	IARG_THREAD_ID,
		        	IARG_MEMORYOP_EA, memOp,
		        	IARG_END);
            }
        }
    }
}


// -------------------------------------------------------------
// Trace instrumentation function
// -------------------------------------------------------------
void I_Trace(TRACE trace, void *v) {

    BOOL isPLT = IsPLT(TRACE_Rtn(trace));

    #if DEBUG_INS    
    printf("-- Instrumenting trace %X of function %s\n", 
        TRACE_Address(trace), RTN_Valid(TRACE_Rtn(trace)) ? RTN_Name(TRACE_Rtn(trace)).c_str() : "<unknown_routine>");
    #endif

    // scan BBLs within the current trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

    	// instrument memory reads and writes
    	for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
    	    Instruction(ins);

        INS tail = BBL_InsTail(bbl);

        // skip system calls
        if ( INS_IsSyscall(tail) ) continue;
        
        // instrument .plt stub calls
        if ( isPLT ) {

            #if DEBUG_INS
            printf("   > .plt stub call\n");
            #endif

            if (gSetup.callingSite) {
                if (gSetup.memBuf)
                    INS_InsertCall(tail, IPOINT_BEFORE, 
                                (AFUNPTR)A_ProcessIndirectCallCSBuf,
                                IARG_FAST_ANALYSIS_CALL,
                                IARG_INST_PTR,
                                IARG_BRANCH_TARGET_ADDR,
                                IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_THREAD_ID,
                                IARG_CONTEXT,
                                IARG_END);
                else
                    INS_InsertCall(tail, IPOINT_BEFORE, 
                                (AFUNPTR)A_ProcessIndirectCallCS,
                                IARG_FAST_ANALYSIS_CALL,
                                IARG_INST_PTR,
                                IARG_BRANCH_TARGET_ADDR,
                                IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_THREAD_ID,
                                IARG_END);
            }
            else {
                if (gSetup.memBuf)
                    INS_InsertCall(tail, IPOINT_BEFORE, 
                                (AFUNPTR)A_ProcessIndirectCallBuf,
                                IARG_FAST_ANALYSIS_CALL,
                                IARG_BRANCH_TARGET_ADDR,
                                IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_THREAD_ID,
                                IARG_CONTEXT,
                                IARG_END);
                else
                    INS_InsertCall(tail, IPOINT_BEFORE, 
                                (AFUNPTR)A_ProcessIndirectCall,
                                IARG_FAST_ANALYSIS_CALL,
                                IARG_BRANCH_TARGET_ADDR,
                                IARG_REG_VALUE, REG_STACK_PTR,
                                IARG_THREAD_ID,
                                IARG_END);
            }
            continue;
        }

        // instrument all calls and returns
        if ( INS_IsCall(tail) ) {
        
            // direct call
            if( INS_IsDirectBranchOrCall(tail) ) {

                // get target address
                ADDRINT target = Target2FunAddr(INS_DirectBranchOrCallTargetAddress(tail));

                #if DEBUG_INS
                printf("   > Direct call to %s\n", Target2RtnName(target).c_str());
                #endif

                // instrument direct call: target address determined here
                if (gSetup.callingSite) {
                    if (gSetup.memBuf)
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessDirectCallCSBuf,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_INST_PTR,
                                              IARG_ADDRINT, target,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_CONTEXT,
                                              IARG_END);
                    else
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessDirectCallCS,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_INST_PTR,
                                              IARG_ADDRINT, target,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_END);
                }
                else {
                    if (gSetup.memBuf)
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessDirectCallBuf,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_ADDRINT, target,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_CONTEXT,
                                              IARG_END);
                    else
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessDirectCall,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_ADDRINT, target,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_END);
                }
            }

            // indirect call: target address determined at call time
            else {

                #if DEBUG_INS
                printf("   > Indirect call\n");
                #endif

                // instrument indirect call
                if (gSetup.callingSite) {
                    if (gSetup.memBuf)
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessIndirectCallCSBuf,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_INST_PTR,
                                              IARG_BRANCH_TARGET_ADDR,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_CONTEXT,
                                              IARG_END);
                    else
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessIndirectCallCS,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_INST_PTR,
                                              IARG_BRANCH_TARGET_ADDR,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_END);
                }
                else {
                    if (gSetup.memBuf)
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessIndirectCallBuf,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_BRANCH_TARGET_ADDR,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_CONTEXT,
                                              IARG_END);
                    else
                        INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                              (AFUNPTR)A_ProcessIndirectCall,
                                              IARG_FAST_ANALYSIS_CALL,
                                              IARG_BRANCH_TARGET_ADDR,
                                              IARG_REG_VALUE, REG_STACK_PTR,
                                              IARG_THREAD_ID,
                                              IARG_END);
                }
            }

            continue;
        }
        
        if ( INS_IsRet(tail) ) {

            #if DEBUG_INS
            printf("   > return\n");
            #endif

            if (gSetup.memBuf)
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                     (AFUNPTR)A_ProcessReturnBuf,
                                     IARG_FAST_ANALYSIS_CALL,
                                     IARG_REG_VALUE, REG_STACK_PTR,
                                     IARG_THREAD_ID,
                                     IARG_CONTEXT,
                                     IARG_END);
            else
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                     (AFUNPTR)A_ProcessReturn,
                                     IARG_FAST_ANALYSIS_CALL,
                                     IARG_REG_VALUE, REG_STACK_PTR,
                                     IARG_THREAD_ID,
                                     IARG_END);
        }
    }
}


// -------------------------------------------------------------
// Application termination handler
// -------------------------------------------------------------
VOID Fini(INT32 code, VOID *v) {
    gSetup.appEnd();
}


// =============================================================
// Public functions
// =============================================================


// -------------------------------------------------------------
// Get name of analyzed application
// -------------------------------------------------------------
// Return pointer to the application name
// Note: the user should *not* deallocate the returned pointer
const char* STool_AppName(){
    return gAppName;
}


// -------------------------------------------------------------
// Get command line of the analyzed application
// -------------------------------------------------------------
// Return pointer to the application name
// Note: the user should *not* deallocate the returned pointer
const char* STool_CommandLine(){
    return gCommandLine;
}


// -------------------------------------------------------------
// STool_RoutineNameByAddr
// -------------------------------------------------------------
// Return pointer to the name of the routine to which the instruction 
// at address rtnAddr belongs, or <unknown routine>, if rtnAddr does not belong to
// any routine.
// Note: the user should *not* deallocate the returned pointer
const char* STool_RoutineNameByAddr(ADDRINT rtnAddr) {
    const string& name = RTN_FindNameByAddress(rtnAddr);
    if (name == "") return "<unknown_routine>";
    else return name.c_str();
}

// -------------------------------------------------------------
// STool_RoutineDemangledNameByAddr
// -------------------------------------------------------------
// Same as STool_RoutineNameByAddr, but names are fully demangled.
const char* STool_RoutineDemangledNameByAddr(ADDRINT rtnAddr, BOOL full) {
    const string& name = RTN_FindNameByAddress(rtnAddr);
    if (name == "") return "<unknown_routine>";
    else return PIN_UndecorateSymbolName(name, 
        full ? UNDECORATION_COMPLETE : UNDECORATION_NAME_ONLY).c_str();
}


// -------------------------------------------------------------
// STool_LibraryIDByAddr
// -------------------------------------------------------------
// Return ID of the library of the routine to which the instruction 
// at address rtnAddr belongs, or STool_INVALID_LIB, if rtnAddr does not belong to
// any routine.
UINT32 STool_LibraryIDByAddr(ADDRINT rtnAddr){

    PIN_LockClient();
    
    const RTN rtn = RTN_FindByAddress(rtnAddr);
    
    UINT32 libID;

    if( RTN_Valid(rtn) ) 
         libID = IMG_Id(SEC_Img(RTN_Sec(rtn)));
    else libID = STool_INVALID_LIB;

    PIN_UnlockClient();

    return libID;
}


// -------------------------------------------------------------
// STool_LibraryNameByID
// -------------------------------------------------------------
// Return pointer to the name of the library with the given ID, 
// or <unknown_library>, if the ID is invalid.
// Note: the user should *not* deallocate the returned pointer
const char* STool_LibraryNameByID(UINT32 libID) {
    IMG img = IMG_FindImgById(libID);
    if( IMG_Valid(img) && strlen(IMG_Name(img).c_str()) > 0 )
         return IMG_Name(img).c_str();
    else return "<unknown_library>";
}


// -------------------------------------------------------------
// STool_LibraryNameByAddr
// -------------------------------------------------------------
// Return pointer to the name of the library of the routine to which the instruction 
// at address rtnAddr belongs, or "<unknown_library>", if rtnAddr does not belong to
// any routine.
// Note: the user should *not* deallocate the returned pointer
const char* STool_LibraryNameByAddr(ADDRINT rtnAddr){
    return STool_LibraryNameByID(STool_LibraryIDByAddr(rtnAddr));
}


// -------------------------------------------------------------
// Strip path from pathname
// -------------------------------------------------------------
const char* STool_StripPath(const char * path) {
    const char* file = strrchr(path,'/');
    if (file) return file + 1;
    else      return path;
}


// -------------------------------------------------------------
// Tool startup function
// -------------------------------------------------------------
void STool_Run(STool_TSetup* s) {

    // make setup configuration globally available
    gSetup = *s;

    // extract application (path)name and full application command line
    for (int i=0; i<gSetup.argc-1;) 
        if (!strcmp("--", gSetup.argv[i++])) {
            int len = 0, j;
            for (j=i; j<gSetup.argc; ++j) len += strlen(gSetup.argv[j])+1;
            gAppName = gSetup.argv[i]; 
            gCommandLine = (char*)calloc(len, 1);
            if (gCommandLine == NULL) exit((printf("Can't allocate command line string\n"),1));
            strcat(gCommandLine, gSetup.argv[i]);
            for (j=i+1; j<gSetup.argc; ++j) {
                strcat(gCommandLine, " ");
                strcat(gCommandLine, gSetup.argv[j]);
            }
        }

    // initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // initialize pin
    if (PIN_Init(gSetup.argc, gSetup.argv)) {
        Usage();
        return;
    }

    // obtain  a key for TLS storage.
    gTlsKey = PIN_CreateThreadDataKey(0);
    
    // if using memory operations buffering, define a BUF_SIZE-pages trace buffer 
    // and register BufferOverflow to be called when the buffer fills up
    if (gSetup.memBuf) buf_id = PIN_DefineTraceBuffer(sizeof(STool_TMemRec), BUF_SIZE, BufferOverflow, 0);

    // register ThreadStart to be called when a thread starts
    PIN_AddThreadStartFunction(ThreadStart, 0);

    // register ThreadEnd to be called when a thread ends
	PIN_AddThreadFiniFunction(ThreadEnd, 0);

    // register I_Trace to be called to instrument traces
    TRACE_AddInstrumentFunction(I_Trace, 0);
   
    // register Fini to be called when application exits
    if (gSetup.appEnd) PIN_AddFiniFunction(Fini, 0);

    // start the program, never returns
    PIN_StartProgram();
}


