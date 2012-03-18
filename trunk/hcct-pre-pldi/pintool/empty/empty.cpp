// =============================================================
// Performance test program: conditional analysis
// =============================================================

// Author:       Camil Demetrescu
// Last changed: $Date: 2010/10/10 09:32:05 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.7 $

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include "pin.H"
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// -------------------------------------------------------------
// Configuration
// -------------------------------------------------------------

#define RTN_INS     1           // routine instrumentation
#define TRACE_INS   2           // trace instrumentation
#define IMG_INS     3           // image instrumentation
#define NULL_INS    4           // no instrumentation function installed
#define INS_MODE    IMG_INS     // instrumentation mode setup

#define DEBUG       0           // if 1, trace analysis operations on stdout
#define DEBUG_INS   0           // if 1, trace instrumentation operations on stdout
#define NO_INS      0           // if 1, no instruction instrumentation is done
#define THREADED    1           // if 1, analysis routines receive the thread ID
#define ANALYSIS_ON 1           // if 0, analysis routines are not executed


// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

static ADDRINT gAnalysisOn = ANALYSIS_ON;
long numEnter = 0, numExit = 0, 
     numInstrumented = 0, numThreads = 0, 
     numPLT = 0, numImg = 0, numSec = 0, numRtn = 0;
UINT64 insTime = 0, procStartTime = 0;


// =============================================================
// Utility functions
// =============================================================


// -------------------------------------------------------------
// get current real time                                                
// -------------------------------------------------------------
static UINT64 os_gettimeofday() { 
	struct timeval tv;
	gettimeofday(&tv, NULL);
	UINT64 t = ((UINT64)tv.tv_sec) * 1000000 + (UINT64)tv.tv_usec;
	return t;
}


// -------------------------------------------------------------
// Print help message                                                
// -------------------------------------------------------------
INT32 Usage() {
    fprintf(stderr, "%s\n", KNOB_BASE::StringKnobSummary().c_str());
    return -1;
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


// =============================================================
// Callback functions
// =============================================================


// -------------------------------------------------------------
// Thread start function
// -------------------------------------------------------------
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {
    fprintf(stderr, "started thread %d\n", threadid);
    numThreads++;
}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
VOID ThreadEnd(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v) {
    fprintf(stderr, "ended thread %d\n", threadid);
}


// =============================================================
// Analysis functions
// =============================================================


// -------------------------------------------------------------
// Function direct call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL 
    A_ProcessDirectCall(
            ADDRINT target, 
            ADDRINT sp
            #if THREADED
            , THREADID threadid
            #endif
        ) 
{
    numEnter++;
}


// -------------------------------------------------------------
// Function indirect call event (without calling site info)
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL 
    A_ProcessIndirectCall(
            ADDRINT target, 
            ADDRINT sp
            #if THREADED
            , THREADID threadid
            #endif
        ) 

{
    A_ProcessDirectCall(
            Target2FunAddr(target), 
            sp 
            #if THREADED
            , threadid
            #endif
        );
}


// -------------------------------------------------------------
// Function exit event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL 
        A_ProcessReturn(
            ADDRINT sp
            #if THREADED
            , THREADID threadid
            #endif
        ) 
{
    numExit++;
}


// -------------------------------------------------------------
// Function exit event
// -------------------------------------------------------------
ADDRINT A_AnalysisOn() {
    return gAnalysisOn;
}


// =============================================================
// Instrumentation functions
// =============================================================

// -------------------------------------------------------------
// Instruction instrumentation function
// -------------------------------------------------------------
VOID I_Instruction(INS ins, BOOL isPLT) {

    #if NO_INS == 0

    // skip system calls
    if ( INS_IsSyscall(ins) ) return;
        
    // instrument .plt stub calls
    if ( isPLT ) {

        #if DEBUG_INS
        printf("   > .plt stub call\n");
        #endif

        numInstrumented++;
        numPLT++;
        
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)A_AnalysisOn, IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE, 
                            (AFUNPTR)A_ProcessIndirectCall,
                            IARG_FAST_ANALYSIS_CALL,
                            IARG_BRANCH_TARGET_ADDR,
                            IARG_REG_VALUE, REG_STACK_PTR,
                            #if THREADED
                            IARG_THREAD_ID,
                            #endif
                            IARG_END);

        return;
    }

    // instrument all calls and returns
    if ( INS_IsCall(ins) ) {
        
        // direct call
        if( INS_IsDirectBranchOrCall(ins) ) {

            // get target address
            ADDRINT target = Target2FunAddr(INS_DirectBranchOrCallTargetAddress(ins));

            #if DEBUG_INS
            printf("   > Direct call\n");
            #endif
            
            numInstrumented++;

            // instrument direct call: target address determined here
            INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)A_AnalysisOn, IARG_END);
            INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE,
                                            (AFUNPTR)A_ProcessDirectCall,
                                            IARG_FAST_ANALYSIS_CALL,
                                            IARG_ADDRINT, target,
                                            IARG_REG_VALUE, REG_STACK_PTR,
                                            #if THREADED
                                            IARG_THREAD_ID,
                                            #endif
                                            IARG_END);
        }

        // indirect call: target address determined at call time
        else {

            #if DEBUG_INS
            printf("   > Indirect call\n");
            #endif

            numInstrumented++;

            // instrument indirect call
            INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)A_AnalysisOn, IARG_END);
            INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE,
                                            (AFUNPTR)A_ProcessIndirectCall,
                                            IARG_FAST_ANALYSIS_CALL,
                                            IARG_BRANCH_TARGET_ADDR,
                                            IARG_REG_VALUE, REG_STACK_PTR,
                                            #if THREADED
                                            IARG_THREAD_ID,
                                            #endif
                                            IARG_END);
        }

        return;
    }
        
    if ( INS_IsRet(ins) ) {

        #if DEBUG_INS
        printf("   > return\n");
        #endif

        numInstrumented++;

        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)A_AnalysisOn, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE,
                                    (AFUNPTR)A_ProcessReturn,
                                    IARG_FAST_ANALYSIS_CALL,
                                    IARG_REG_VALUE, REG_STACK_PTR,
                                    #if THREADED
                                    IARG_THREAD_ID,
                                    #endif
                                    IARG_END);
    }

    #endif
}


// -------------------------------------------------------------
// Routine instrumentation function
// -------------------------------------------------------------
VOID I_Routine(RTN rtn, VOID *v) {

    numRtn++;

    #if NO_INS == 0

    RTN_Open(rtn);

    if (IsPLT(rtn))
        I_Instruction(BBL_InsTail(RTN_BblTail(rtn)), true);
    else 
        // Scan instructions of the routine
        for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
            I_Instruction(ins, false);

    RTN_Close(rtn);

    #endif
}


// -------------------------------------------------------------
// Trace instrumentation function
// -------------------------------------------------------------
void I_Trace(TRACE trace, void *v) {

    #if NO_INS == 0

    BOOL isPLT = IsPLT(TRACE_Rtn(trace));

    #if DEBUG_INS    
    printf("-- Instrumenting trace %X of function %s\n", 
        TRACE_Address(trace), RTN_Valid(TRACE_Rtn(trace)) ? RTN_Name(TRACE_Rtn(trace)).c_str() : "<unknown_routine>");
    #endif

    // scan BBLs within the current trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        I_Instruction(BBL_InsTail(bbl), isPLT);
    }

    #endif
}


// -------------------------------------------------------------
// Image instrumentation function
// -------------------------------------------------------------
VOID I_ImageLoad(IMG img, VOID *v) {
        
    UINT64 start = os_gettimeofday();

    numImg++;

    fprintf(stderr, "loaded image %s\n", IMG_Name(img).c_str());

    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
        numSec++;
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
            I_Routine(rtn, 0);
    }

    insTime += os_gettimeofday() - start;
}


// -------------------------------------------------------------
// Application termination handler
// -------------------------------------------------------------
VOID Fini(INT32 code, VOID *v) {

    UINT64 procElapsedTime = os_gettimeofday() - procStartTime;

    fprintf(stderr, "numEnter=%lu - numExit=%lu - numInstrumented=%lu - "
                    "numThreads=%lu - numImg=%lu - numSec=%lu - "
                    "numRtn=%lu - numPLTs=%lu\n", 
        numEnter, numExit, numInstrumented, numThreads, numImg, numSec, numRtn, numPLT);

    fprintf(stderr, "elapsed time=%.2fs - instr time=%.2fs - analysis+exec time=%.2fs\n", 
        procElapsedTime/1000000.0, insTime/1000000.0, (procElapsedTime-insTime)/1000000.0);
}


// -------------------------------------------------------------
// Tool startup function
// -------------------------------------------------------------
int main(int argc, char** argv) {

    procStartTime = os_gettimeofday();

    // initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // initialize pin
    if (PIN_Init(argc, argv)) {
        Usage();
        return 1;
    }

    // register ThreadStart to be called when a thread starts
    PIN_AddThreadStartFunction(ThreadStart, 0);

    // register ThreadEnd to be called when a thread ends
    PIN_AddThreadFiniFunction(ThreadEnd, 0);

    #if INS_MODE == TRACE_INS

    // register callback to be called to instrument traces
    TRACE_AddInstrumentFunction(I_Trace, 0);

    #elif INS_MODE == RTN_INS

    // Register callback to be called to instrument rtn
    RTN_AddInstrumentFunction(I_Routine, 0);

    #elif INS_MODE == IMG_INS

    // Register callback to be called when each image is loaded
    IMG_AddInstrumentFunction(I_ImageLoad, 0);

    #endif
   
    // register Fini to be called when application exits
    PIN_AddFiniFunction(Fini, 0);

    // start the program, never returns
    PIN_StartProgram();

    return 0;
}


