// =============================================================
// This tool computes the calling context tree
// =============================================================

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pin.H"


// -------------------------------------------------------------
// Performance tests
// -------------------------------------------------------------
// inkscape (about 190M calls, 47777 routines)
// - without pin:
//       user 0m1.484s - sys 0m2.380s
// - with EMPTY_ANALYSIS == 1 && RTN_INSTR == 0 && RESOLVE_TARGET == 0 && CALLING_SITE == 0:
//       user 0m28.682s - sys 0m8.597s (just trace instrumentation cost - slowdown 19x)
// - with EMPTY_ANALYSIS == 1 && RTN_INSTR == 0 && RESOLVE_TARGET == 0 && CALLING_SITE == 0 && -inline 0:
//       user 0m36.918s - sys 0m8.697s (just trace instrumentation cost - slowdown 25x)
// - with EMPTY_ANALYSIS == 1 && RTN_INSTR == 1 && RESOLVE_TARGET == 0 && CALLING_SITE == 0:
//       user 0m37.798s - sys 0m11.573s (just routine + partial trace instrumentation cost - slowdown 26x)
// - with EMPTY_ANALYSIS == 0 && RTN_INSTR == 1 && RESOLVE_TARGET == 1 && CALLING_SITE == 0
//       user 2m40.914s - sys 0m20.017s (slowdown 108x)
// - with EMPTY_ANALYSIS == 0 && RTN_INSTR == 0 && RESOLVE_TARGET == 1 && CALLING_SITE == 0
//       user 2m32.286s - sys 0m17.761s (slowdown 103x)
// - with EMPTY_ANALYSIS == 0 && RTN_INSTR == 1 && RESOLVE_TARGET == 0 && CALLING_SITE == 0
//       user 0m52.123s - sys 0m14.257s (slowdown 33x)
// - with EMPTY_ANALYSIS == 0 && RTN_INSTR == 0 && RESOLVE_TARGET == 0 && CALLING_SITE == 0
//       user 0m44.663s - sys 0m11.549s (slowdown 30x)



// -------------------------------------------------------------
// Macros
// -------------------------------------------------------------

#define LOG2(x) ((int)(log(x)/log(2)))


// -------------------------------------------------------------
// Constants
// -------------------------------------------------------------

#define PAD_SIZE        (128-2*sizeof(UINT32)-sizeof(ActivationRec*)-sizeof(THREADID))
                                // padding size (avoiding false sharing performance degrading)

#define INIT_STACK      64      // initial stack size
#define DEBUG           0       // if 1, trace analysis operations on stdout
#define DEBUG_INS       0       // if 1, trace instrumentation operations on stdout
#define DUMP_STAT       1       // if 1, compute and write statistics about the cct to file
#define DUMP_TREE       0       // if 1, dump to file the cct
#define TAB             4       // tab size for cct indentation
#define RTN_INSTR       0       // if 1, instrument calls/rts at RTN level (but plt exits, made at TRACE LEVEL)
                                // if 0, instrument calls/rts at TRACE level
#define RTN_STAT        1       // if 1 or RTN_INSTR==1, compute routine statistics
#define EMPTY_ANALYSIS  0       // if 1, instrumentation analysis routines are empty (performance benchmark reference)

#ifndef RESOLVE_TARGET
#define RESOLVE_TARGET  0       // if 1, let targets of calls always point to the routine start address
#endif

#ifndef CALLING_SITE
#define CALLING_SITE    0       // if 1, treat calls from different calling sites as different
#endif


// -------------------------------------------------------------
// Structures
// -------------------------------------------------------------

// Record associated with CCT node
typedef struct CCTNode {
    struct CCTNode* firstChild;   // first child of the node in the CCT
    struct CCTNode* nextSibling;  // next sibling of the node in the CCT
    UINT64          count;        // number of times the context is reached 
    ADDRINT         target;       // address of call target within function associated with CCT node
                                  // always coincides with function address if RESOLVE_TARGET == 1
    #if CALLING_SITE
    ADDRINT         callSite;     // address of calling instruction
    #endif
} CCTNode;

// Record for global statistics about the CCT
typedef struct CCTStat {
    UINT64 numNodes;    // number of nodes of the CCT
    UINT64 numCalls;    // total number of routine calls
    UINT64 numIntCalls; // total number of calls to internal nodes
    UINT64 numLeaves;   // number of leaves of the CCT
    UINT64 numDegr1;    // number of nodes of degree 1
    UINT64 totDepth;    // sum of depths of all leaves
    UINT64 totDegr;     // sum of degrees of all internal nodes 
    UINT64 totWDegr;    // weighted sum of degrees of all internal nodes
    UINT32 maxDepth;    // maximum depth of a leaf
    UINT32 maxDegr;     // maximum degree of an internal node
    UINT32 callDis[32]; // callDis[i] = number of nodes called x times, with x in [2^i, 2^{i+1})
    UINT64 callAgg[32]; // callAgg[i] = sum of call counts of nodes called x times, with x in [2^i, 2^{i+1})
    UINT32 degrDis[32]; // degrDis[i] = number of nodes with degree d in [2^i, 2^{i+1})
    UINT32 freqDis[64]; // freqDis[i] = number of nodes called x times, with x >= numCalls/2^{i}
} CCTStat;

// Call stack activation record
typedef struct ActivationRec {
    CCTNode* node;      // pointer to the CCT node associated with the call
    ADDRINT  currentSP; // stack pointer register content before the call
} ActivationRec;

// Thread local data record
typedef struct ThreadRec {
    ActivationRec* callStack;
    UINT32         callStackTop;
    UINT32         callStackSize;
    THREADID       threadID;
    UINT8          pad[PAD_SIZE];   // to avoid false sharing problems
} ThreadRec;


// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

static int      ArgC;               // command line token count
static char**   ArgV;               // command line tokens
static TLS_KEY  tlsKey;             // key for accessing TLS storage in the threads. initialized once in main()
static UINT32   numRTN;             // total number of routines (except plt's)
static UINT32   numDirectCall;      // total number of instrumented direct calls made outside plt's
static UINT32   numIndirectCall;    // total number of instrumented indirect calls


// =============================================================
// Utility functions
// =============================================================


// -------------------------------------------------------------
// Print help message                                                
// -------------------------------------------------------------
static INT32 Usage() {
    fprintf(stderr, "This Pintool creates the calling context tree of a running program\n");
    fprintf(stderr, "%s\n", KNOB_BASE::StringKnobSummary().c_str());
    return -1;
}

// -------------------------------------------------------------
// Strip path from pathname
// -------------------------------------------------------------
static const char* StripPath(const char * path) {
    const char * file = strrchr(path,'/');
    if (file) return file + 1;
    else      return path;
}


// -------------------------------------------------------------
// Get routine name from address
// -------------------------------------------------------------
static const string& Target2RtnName(ADDRINT target) {
  const string & name = RTN_FindNameByAddress(target);
  if (name == "") return *new string("<unknown routine>");
  else return *new string(name);
}


// -------------------------------------------------------------
// Get library name from address
// -------------------------------------------------------------
const string& Target2LibName(ADDRINT target) {
    PIN_LockClient();
    
    const RTN rtn = RTN_FindByAddress(target);
    static const string _invalid_rtn("<unknown image>");
    string name;
    
    if( RTN_Valid(rtn) ) name = IMG_Name(SEC_Img(RTN_Sec(rtn)));
    else name = _invalid_rtn;

    PIN_UnlockClient();
    return *new string(name);
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
static inline ThreadRec* getTLS(THREADID threadid) {
    ThreadRec* tdata = static_cast<ThreadRec*>(PIN_GetThreadData(tlsKey, threadid));
    return tdata;
}


// -------------------------------------------------------------
// Deallocate calling context tree
// -------------------------------------------------------------
static VOID freeTree(CCTNode* root) {

    // skip empty subtrees
    if (root == NULL) return;

    // call recursively function on children
    for (CCTNode* theNodePtr = root->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling) freeTree(theNodePtr);

    // deallocate CCT node
    free(root);
}


// -------------------------------------------------------------
// Get size of hot calling context tree (HCCT), i.e. a subtree of the CCT
// obtained by pruning all non-hot nodes with no hot descendants.
// A node is in the HCCT if has a counter >= threshold or has a hot descendant
// -------------------------------------------------------------
static UINT64 hotTreeSize(CCTNode* root, UINT32 threshold) {

    // skip empty subtrees
    if (root == NULL) return 0;

    // account for node, if hot
    UINT64 hotSize = 0;

    // call recursively function on children
    for (CCTNode* theNodePtr = root->firstChild; theNodePtr != NULL; theNodePtr = theNodePtr->nextSibling)
        hotSize += hotTreeSize(theNodePtr, threshold);

    // if the root has a hot descendant or has a counter above the threshold, then count the root
    if (hotSize > 0 || root->count >= threshold) return hotSize + 1;
    
    return hotSize;
}


// -------------------------------------------------------------
// Build hot calling context tree (HCCT) - see hotTreeSize()
// -------------------------------------------------------------
static BOOL pruneTree(CCTNode* root, UINT32 threshold) {

    BOOL     hot  = FALSE;
    CCTNode* prev = NULL;

    // skip empty subtrees
    if (root == NULL) return FALSE;

    // call recursively function on children
    for (CCTNode* theNodePtr = root->firstChild; theNodePtr != NULL;) {

        CCTNode* succ = theNodePtr->nextSibling;

        BOOL hotChild = pruneTree(theNodePtr, threshold);

        if (!hotChild) {
            if (!prev) root->firstChild  = succ;
            else       prev->nextSibling = succ;
            free(theNodePtr);
        }
        else {
            hot  = TRUE;
            prev = theNodePtr;
        }
        
        theNodePtr = succ;
    }

    // CCT node has to be removed
    if (hot || root->count >= threshold) return TRUE;
    
    return FALSE;
}


// -------------------------------------------------------------
// Dump calling context tree
// -------------------------------------------------------------
static VOID dumpTree(CCTNode* root, UINT32 indent, FILE* out) {

    // skip empty subtrees
    if (root == NULL) return;

    // indent line
    for (UINT32 i=0; i<indent; ++i) 
        if (i + TAB < indent) fprintf(out, (i % TAB) ? " " : "|");
        else fprintf(out, (i % TAB) ? ( (i < indent - 1 ) ? " " : ( i < indent ? " " : " ")) : "|");
        
    // print function name and context count
    fprintf(out, "%s (%llu)\n", Target2RtnName(root->target).c_str(), root->count);

    // call recursively function on children
    for (CCTNode* theNodePtr = root->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling) dumpTree(theNodePtr, indent + TAB, out);
}


// -------------------------------------------------------------
// Count total number of calls
// -------------------------------------------------------------
static UINT32 numCalls(CCTNode* root) {

    // skip empty subtrees
    if (root == NULL) return 0;

    // start from root node count
    UINT32 count = root->count;

    // call recursively function on children adding counts of subtrees
    for (CCTNode* theNodePtr = root->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling) count += numCalls(theNodePtr);

    return count;
}


// -------------------------------------------------------------
// Compute statistics of calling context tree
// -------------------------------------------------------------
static VOID getStat(CCTNode* root, CCTStat* stat) {

    // reset statistics
    memset(stat, 0, sizeof(CCTStat));

    // empty tree: skip
    if (root == NULL) return;

    // count total number of calls (sum of counters of nodes in the CCT)
    // this is needed later on to compute freqDis[]
    stat->numCalls = numCalls(root);

    // stack record
    typedef struct StackRec {
        struct StackRec* next;
        CCTNode*         node;
        UINT32           depth;
    } StackRec;

    // stack operations
    #define push(s,n,d)                                             \
        do {                                                        \
            StackRec* temp = (StackRec*)malloc(sizeof(StackRec));   \
            if (temp == NULL) exit((printf("out of memory\n"),1));  \
            temp->next  = (s);                                      \
            temp->node  = (n);                                      \
            temp->depth = (d);                                      \
            (s)         = temp;                                     \
        } while(0)

    #define pop(s,n,d)                                              \
        do {                                                        \
            StackRec* temp = (s);                                   \
            (s) = temp->next;                                       \
            (n) = temp->node;                                       \
            (d) = temp->depth;                                      \
            free(temp);                                             \
        } while(0)

    StackRec* stack = NULL;

    // push root
    push(stack, root, 1);

    while (stack != NULL) {

        UINT32 degr = 0, depth;
    
        pop(stack, root, depth);

        // schedule subtrees for scanning
        for (CCTNode* child = root->firstChild;
             child != NULL;
             child = child->nextSibling) {

             degr++;
             push(stack, child, depth + 1);
        }

        // count nodes
        stat->numNodes++;
        
        // compute distribution of call counts of nodes 
        // callDis[i] = # of CCT nodes that were reached x times, with x in [2^i, 2^(i+1))
        stat->callDis[LOG2(root->count)]++;
        stat->callAgg[LOG2(root->count)] += root->count;

        // compute distribution of call counts of nodes as a fraction of the total number of calls (already computed)
        UINT64 N = stat->numCalls;
        for (int i = 0; i<64 && N > 0; i++, N >>= 1)
            if (root->count >= N) stat->freqDis[i]++;

        // node is internal
        if (degr) {

            // compute distribution of internal node degrees        
            // degrDis[i] = # of CCT nodes having degree d, with d in [2^i, 2^(i+1))
            stat->degrDis[LOG2(degr)]++;

            stat->totDegr  += degr;             // accumulate degree of internal node
            stat->totWDegr += degr*root->count; // accumulate weighted degree of internal node
            stat->numIntCalls += root->count;   // count number of calls to internal nodes
            if (degr > stat->maxDegr ) 
                stat->maxDegr = degr;           // compute max degree
            if (degr == 1) 
                stat->numDegr1++;               // count nodes of degree 1
        }
    
        // node is a leaf
        else {
            stat->numLeaves++;                  // count leaf
            stat->totDepth += depth;            // accumulate depth of leaf
            if (depth > stat->maxDepth)
                stat->maxDepth = depth;         // compute max depth depth        
        }
    }

    #undef push
    #undef pop
}


// -------------------------------------------------------------
// Write output to file
// -------------------------------------------------------------
VOID WriteOutput(ThreadRec* tdata, FILE* out) {

    #if DUMP_STAT
    CCTStat stat;

    // compute CCT statistics
    getStat(tdata->callStack[0].node, &stat);

    // dump CCT statistics
    fprintf(out, "----------\nApplication: ");
    for (int i=0; i<ArgC-1;) 
        if (!strcmp("--", ArgV[i++])) 
             for (; i<ArgC; i++) fprintf(out, "%s ", ArgV[i]);
    fprintf(out, "- Thread ID: %u - RESOLVE_TARGET = %d - CALLING_SITE = %d\n\n", tdata->threadID, RESOLVE_TARGET, CALLING_SITE);

    if (numRTN) fprintf(out, "Number of routines: %u\n", numRTN);
    fprintf(out, "Number of calls: %llu\n", stat.numCalls);
    fprintf(out, "Number of nodes of the CCT: %llu\n", stat.numNodes);
    fprintf(out, "Number of leaves of the CCT: %llu (%.0f%%)\n", stat.numLeaves, 100.0*stat.numLeaves/stat.numNodes);
    fprintf(out, "Number of internal nodes of the CCT: %llu (%.0f%%)\n", stat.numNodes-stat.numLeaves, 100.0*(stat.numNodes-stat.numLeaves)/stat.numNodes);
    if (numRTN) {
        fprintf(out, "\nAverage number of direct calls per routine: %.1f\n", (double)numDirectCall/numRTN);
        fprintf(out, "Average number of indirect calls per routine: %.1f\n", (double)numIndirectCall/numRTN);
    }

    fprintf(out, "\nCCT maximum depth: %u\n",  stat.maxDepth);
    fprintf(out, "CCT average depth of a leaf: %.0f\n", stat.numLeaves > 0 ? (double)stat.totDepth/stat.numLeaves : 0.0);
    fprintf(out, "CCT maximum degree: %u\n", stat.maxDegr);
    fprintf(out, "CCT average degree of an internal node: %.2f\n", (stat.numNodes-stat.numLeaves) > 0 ? (double)stat.totDegr/(stat.numNodes-stat.numLeaves) : 0.0);
    fprintf(out, "CCT average weighted degree of an internal node: %.2f\n", (stat.numIntCalls) > 0 ? (double)stat.totWDegr/stat.numIntCalls : 0.0);

    // find max j for which stat.callDis[j] > 0
    int j;
    for (j=31; j>=0 && !stat.callDis[j]; j--);

    fprintf(out, "\nDistribution of CCT node calls - aggregate (cct nodes accounting for given fractions of calls):\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s %-14s %-14s %-14s %-15s %-15s %-15s\n", 
                 "i", "% nodes", "% calls", "# nodes", "# calls", "min node freq", "(% of calls)", "# HCCT nodes", "# cold in HCCT", "% cold in HCCT", "space saving %");
    UINT64 callSum = 0;
    UINT64 nodeSum = 0;
    for (int i=j; i>=0; i--) {
        UINT64 hcctSize = hotTreeSize(tdata->callStack[0].node ,1<<i);
        callSum += stat.callAgg[i];
        nodeSum += stat.callDis[i];
        fprintf(out, "%-4d %-14.2f %-14.2f %-14llu %-14llu %-14u %-14.6f %-14llu %-15llu %-15.2f %-15.2f\n", 
            i, 100.0*nodeSum/stat.numNodes, 100.0*callSum/stat.numCalls, 
            nodeSum, callSum, 1<<i, 100.0*(1<<i)/stat.numCalls, 
            hcctSize, hcctSize - nodeSum, 100.0*(hcctSize - nodeSum)/hcctSize, 100.0*(stat.numNodes-hcctSize)/stat.numNodes);
    }

    fprintf(out, "\nDistribution of CCT node calls as a fraction of the total number of calls N:\n\n");
    fprintf(out, "%-4s %-21s  %-14s %-14s\n", 
                 "i", "threshold t %", "# nodes >= tN", "% nodes >= tN");
    for (int i=0; i<64 && (i==0 || stat.freqDis[i-1]<stat.numNodes); ++i)
        fprintf(out, "%-4d %21.17f  %-14u %-14.3f\n", 
            i, 100.0/(double)((UINT64)1 << i), stat.freqDis[i], 
            (stat.numNodes) > 0 ? 100.0*stat.freqDis[i]/(stat.numNodes) : 0.0);

    fprintf(out, "\nDistribution of CCT node calls:\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s\n", 
                 "i", "calls from", "calls to", "# nodes", "% nodes");
    for (int i=0; i<=j; ++i)
        fprintf(out, "%-4d %-14u %-14u %-14u %-14.1f\n", 
            i, 1<<i, (1<<(i+1))-1, stat.callDis[i], 
            (stat.numNodes) > 0 ? 100.0*stat.callDis[i]/(stat.numNodes) : 0.0);

    fprintf(out, "\nDistribution of CCT node degrees:\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s\n", 
                 "i", "degree from", "degree to", "# int. nodes", "% int. nodes");
    for (j=31; j>=0 && !stat.degrDis[j]; j--);
    for (int i=0; i<=j; ++i)
        fprintf(out, "%-4d %-14u %-14u %-14u %-14.1f\n", 
            i, 1<<i, (1<<(i+1))-1, stat.degrDis[i],
            (stat.numNodes-stat.numLeaves) > 0 ? 100.0*stat.degrDis[i]/(stat.numNodes-stat.numLeaves) : 0.0);
    #endif

    #if DUMP_TREE
    fprintf(out, "----------\nCCT dump:\n");
    dumpTree(tdata->callStack[0].node, 0, out);
    #endif
}


// =============================================================
// Callback functions
// =============================================================

// -------------------------------------------------------------
// Thread start function
// -------------------------------------------------------------
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {

    #if DEBUG
    printf("[TID=%u] Starting new thread\n", threadid);
    #endif

    ThreadRec* tdata = (ThreadRec*)malloc(sizeof(ThreadRec));
    if (tdata == NULL) exit((printf("[TID=%u] Can't allocate thread specific info\n", threadid),1));

    tdata->threadID      = threadid;
    tdata->callStackTop  = 0;
    tdata->callStackSize = INIT_STACK;
    tdata->callStack     = (ActivationRec*)malloc(tdata->callStackSize*sizeof(ActivationRec));
    if (tdata->callStack == NULL) exit((printf("[TID=%u] Can't allocate stack\n", threadid),1));

    // allocate sentinel
    CCTNode* theNodePtr = (CCTNode*)calloc(sizeof(CCTNode), 1);
    if (theNodePtr == NULL) exit((printf("[TID=%u] Can't allocate stack node\n", threadid),1));
    theNodePtr->count = 1;

    // make stack sentinel
    tdata->callStack[tdata->callStackTop].node = theNodePtr;    

    // associate thread-specific data to thread
    PIN_SetThreadData(tlsKey, tdata, threadid);
}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
VOID ThreadEnd(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v) {

    FILE*  out;
    char fileName[32]; 
    const char *appName = (char*)"?";

    // get pointer to thread local data
    ThreadRec* tdata = getTLS(threadid);
    
    // find application name
    for (int i=0; i<ArgC-1;) 
        if (!strcmp("--", ArgV[i++])) { appName = StripPath(ArgV[i]); }

    // make output file name
    #if CALLING_SITE
    sprintf(fileName, "cct-%s-%u.cs.out", appName, threadid);
    #else
    sprintf(fileName, "cct-%s-%u.out", appName, threadid);
    #endif

    // print message
    printf("[%s] creating output file...", fileName);

    // create output file
    out = fopen(fileName, "w");
    if (out == NULL) exit((printf("can't create output file %s\n", fileName), 1));

    // write output file
    WriteOutput(tdata, out);

    // close output file
    fclose(out);

    // free CCT
    freeTree(tdata->callStack[0].node);

    // free call stack
    free(tdata->callStack);

    // free thread local data
    free(tdata);

    // print message
    printf(" done.\n");
}


// =============================================================
// Analysis functions
// =============================================================

// -------------------------------------------------------------
// Roll back stack if we got here from a longjmp
// (Stack grows down and register stack grows up)
// -------------------------------------------------------------
static inline VOID AdjustStack(ThreadRec* tdata, ADDRINT currentSP) {
  
    // roll back stack
    while ( tdata->callStackTop >= 1 && 
            currentSP >= tdata->callStack[tdata->callStackTop].currentSP ) {

        #if DEBUG
        printf("[TID=%u] *** longjmp: popping activation of %s\n", 
            tdata->threadID, 
            Target2RtnName(tdata->callStack[tdata->callStackTop].node->target).c_str());
        #endif

        tdata->callStackTop--; // pop activation
    }
}


// -------------------------------------------------------------
// Function enter event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessCall(
    #if CALLING_SITE
    ADDRINT ip, 
    #endif
    ADDRINT target, ADDRINT sp, THREADID threadid) {

    #if EMPTY_ANALYSIS == 0

    CCTNode* theNodePtr;

    // get thread local data
    ThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStack(tdata, sp);

    // get address of function containing target address
    #if RESOLVE_TARGET
    target = Target2FunAddr(target);
    #endif

    // did we already encounter this context?
    for (theNodePtr = tdata->callStack[tdata->callStackTop].node->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling)
            #if CALLING_SITE
            if (theNodePtr->callSite == ip && theNodePtr->target == target) goto Exit;
            #else
            if (theNodePtr->target == target) goto Exit;
            #endif

    // create new context node
    theNodePtr = (CCTNode*)calloc(sizeof(CCTNode), 1);
    if (theNodePtr == NULL) exit((printf("[TID=%u] Out of memory error\n", threadid),1));
    
    // add node to tree
    theNodePtr->nextSibling = tdata->callStack[tdata->callStackTop].node->firstChild;
    tdata->callStack[tdata->callStackTop].node->firstChild = theNodePtr;
    theNodePtr->target = target;
    #if CALLING_SITE
    theNodePtr->callSite = ip;
    #endif

  Exit:

    // increase activations counter
    tdata->callStackTop++;

    // possibly expand stack
    if (tdata->callStackTop >= tdata->callStackSize) {
        tdata->callStackSize <<= 1;
        tdata->callStack = (ActivationRec*)realloc(tdata->callStack, tdata->callStackSize*sizeof(ActivationRec)); 
        if (tdata->callStack == NULL) exit((printf("[TID=%u] Can't expand stack\n", threadid),1));
    }    

    // push current context node to stack
    tdata->callStack[tdata->callStackTop].node      = theNodePtr;
    tdata->callStack[tdata->callStackTop].currentSP = sp;

    #if DEBUG
    printf("[TID=%u] Entering %s - # activations = %d - stack size = %d\n", 
        threadid, Target2RtnName(theNodePtr->target).c_str(), tdata->callStackTop, tdata->callStackSize);
    #endif

    // increase context counter
    theNodePtr->count++;
    
    #endif
}


// -------------------------------------------------------------
// Function exit event
// -------------------------------------------------------------
VOID PIN_FAST_ANALYSIS_CALL A_ProcessReturn(ADDRINT sp, THREADID threadid) {

    #if EMPTY_ANALYSIS == 0

    ThreadRec* tdata = getTLS(threadid);

    // roll back stack in case of longjmp
    AdjustStack(tdata, sp);

    if ( tdata->callStackTop < 1 )
        exit((printf("[TID=%u] Internal error: stack bottomed out\n", threadid), 1));

    #if DEBUG
    printf("[TID=%u] Leaving %s\n", 
        threadid,
        Target2RtnName(tdata->callStack[tdata->callStackTop].node->target).c_str());
    #endif

    // pop entry
    tdata->callStackTop--;

    #if DEBUG
    if (tdata->callStackTop > 0)
        printf("[TID=%u] Back to %s - # activations = %d - stack size = %d\n", 
            threadid, 
            Target2RtnName(tdata->callStack[tdata->callStackTop].node->target).c_str(), 
            tdata->callStackTop, 
            tdata->callStackSize);
    else printf("[TID=%u] Back to stack bottom\n", threadid);

    #endif

    #endif
}


// =============================================================
// Instrumentation functions
// =============================================================


#if RTN_STAT || RTN_INSTR

// -------------------------------------------------------------
// Routine instrumentation function (covers all functions but plt's)
// -------------------------------------------------------------
VOID I_Routine(RTN rtn, VOID *v) {

    #if DEBUG_INS
    printf("## Instrumenting routine: %s - section: %s\n", 
        RTN_Name(rtn).c_str(), StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str()));
    #endif

    assert(!IsPLT(rtn));

    // increase routine counter
    numRTN++;

    // open routine for instrumentation
    RTN_Open(rtn);

    // scan instructions within the current routine
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {

        // skip system calls
        if ( INS_IsSyscall(ins) ) continue;
        
        // Instrument all calls and returns
        if ( INS_IsCall(ins) ) {
        
            // Direct call
            if( INS_IsDirectBranchOrCall(ins) ) {

                // Get target address
                #if RTN_INSTR || DEBUG_INS
                ADDRINT target = Target2FunAddr(INS_DirectBranchOrCallTargetAddress(ins));
                #endif

                #if DEBUG_INS
                printf("   > Direct call to %s\n", Target2RtnName(target).c_str());
                #endif

                // increase direct call counter
                numDirectCall++;

                // Indirect call: target address determined here
                #if RTN_INSTR
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                                         (AFUNPTR)A_ProcessCall,
                                         IARG_FAST_ANALYSIS_CALL,
                                         #if CALLING_SITE
                                         IARG_INST_PTR,
                                         #endif
                                         IARG_ADDRINT, target,
                                         IARG_REG_VALUE, REG_STACK_PTR,
                                         IARG_THREAD_ID,
                                         IARG_END);
                #endif
            }

            // Indirect call: target address determined at call time
            else {

                #if DEBUG_INS
                printf("   > Indirect call\n");
                #endif
    
                // increase indirect call counter
                numIndirectCall++;

                // Instrument call
                #if RTN_INSTR
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                                         (AFUNPTR)A_ProcessCall,
                                         IARG_FAST_ANALYSIS_CALL,
                                         #if CALLING_SITE
                                         IARG_INST_PTR,
                                         #endif
                                         IARG_BRANCH_TARGET_ADDR,
                                         IARG_REG_VALUE, REG_STACK_PTR,
                                         IARG_THREAD_ID,
                                         IARG_END);
                #endif
            }
        }
        
        if ( INS_IsRet(ins) ) {

            #if DEBUG_INS
            printf("   > return\n");
            #endif

            #if RTN_INSTR
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                                     (AFUNPTR)A_ProcessReturn,
                                     IARG_FAST_ANALYSIS_CALL,
                                     IARG_REG_VALUE, REG_STACK_PTR,
                                     IARG_THREAD_ID,
                                     IARG_END);
            #endif
        }
    }

    RTN_Close(rtn);
}

#endif


#if RTN_INSTR

// -------------------------------------------------------------
// PLT Trace instrumentation function
// -------------------------------------------------------------
void I_PLTTrace(TRACE trace, void *v) {

    // FIXME if (PIN_IsSignalHandler()) {Sequence_ProcessSignalHandler(head)};

    // skip traces that do not belong to a .plt
    if (!IsPLT(TRACE_Rtn(trace))) return;

    #if DEBUG_INS    
    printf("-- Instrumenting trace %X of function %s\n", 
        TRACE_Address(trace), RTN_Valid(TRACE_Rtn(trace)) ? RTN_Name(TRACE_Rtn(trace)).c_str() : "<unknown function>");
    #endif

    // scan BBLs within the current trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

        INS tail = BBL_InsTail(bbl);
       
        #if DEBUG_INS
        printf("   > .plt stub call\n");
        #endif

        INS_InsertCall(tail, IPOINT_BEFORE, 
                       (AFUNPTR)A_ProcessCall,
                       IARG_FAST_ANALYSIS_CALL,
                       #if CALLING_SITE
                       IARG_INST_PTR,
                       #endif
                       IARG_BRANCH_TARGET_ADDR,
                       IARG_REG_VALUE, REG_STACK_PTR,
                       IARG_THREAD_ID,
                       IARG_END);
    }
}

#else

// -------------------------------------------------------------
// Trace instrumentation function
// -------------------------------------------------------------
void I_Trace(TRACE trace, void *v) {

    // FIXME if (PIN_IsSignalHandler()) {Sequence_ProcessSignalHandler(head)};

    BOOL isPLT = IsPLT(TRACE_Rtn(trace));

    #if DEBUG_INS    
    printf("-- Instrumenting trace %X of function %s\n", 
        TRACE_Address(trace), RTN_Valid(TRACE_Rtn(trace)) ? RTN_Name(TRACE_Rtn(trace)).c_str() : "<unknown function>");
    #endif

    // scan BBLs within the current trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

        INS tail = BBL_InsTail(bbl);

        // skip system calls
        if ( INS_IsSyscall(tail) ) continue;
        
        // Instrument all calls and returns
        if ( INS_IsCall(tail) ) {
        
            // Direct call
            if( INS_IsDirectBranchOrCall(tail) ) {

                // Get target address
                ADDRINT target = Target2FunAddr(INS_DirectBranchOrCallTargetAddress(tail));

                #if DEBUG_INS
                printf("   > Direct call to %s\n", Target2RtnName(target).c_str());
                #endif

                // Indirect call: target address determined here
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                         (AFUNPTR)A_ProcessCall,
                                         IARG_FAST_ANALYSIS_CALL,
                                         #if CALLING_SITE
                                         IARG_INST_PTR,
                                         #endif
                                         IARG_ADDRINT, target,
                                         IARG_REG_VALUE, REG_STACK_PTR,
                                         IARG_THREAD_ID,
                                         IARG_END);
            }

            // Indirect call: target address determined at call time
            else if ( !isPLT ) {

                #if DEBUG_INS
                printf("   > Indirect call\n");
                #endif

                // Instrument call
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                         (AFUNPTR)A_ProcessCall,
                                         IARG_FAST_ANALYSIS_CALL,
                                         #if CALLING_SITE
                                         IARG_INST_PTR,
                                         #endif
                                         IARG_BRANCH_TARGET_ADDR,
                                         IARG_REG_VALUE, REG_STACK_PTR,
                                         IARG_THREAD_ID,
                                         IARG_END);
            }
        }
        
        if ( isPLT ) {

            #if DEBUG_INS
            printf("   > .plt stub call\n");
            #endif

            INS_InsertCall(tail, IPOINT_BEFORE, 
                           (AFUNPTR)A_ProcessCall,
                           IARG_FAST_ANALYSIS_CALL,
                           #if CALLING_SITE
                           IARG_INST_PTR,
                           #endif
                           IARG_BRANCH_TARGET_ADDR,
                           IARG_REG_VALUE, REG_STACK_PTR,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        
        if ( INS_IsRet(tail) ) {

            #if DEBUG_INS
            printf("   > return\n");
            #endif

            INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                                     (AFUNPTR)A_ProcessReturn,
                                     IARG_FAST_ANALYSIS_CALL,
                                     IARG_REG_VALUE, REG_STACK_PTR,
                                     IARG_THREAD_ID,
                                     IARG_END);
        }
    }
}

#endif


// =============================================================
// Main function of the CCT pin tool
// =============================================================

int main(int argc, char * argv[]) {

    ArgC = argc;
    ArgV = argv;

    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Obtain  a key for TLS storage.
    tlsKey = PIN_CreateThreadDataKey(0);

    // Register ThreadStart to be called when a thread starts
    PIN_AddThreadStartFunction(ThreadStart, 0);

    // Register ThreadEnd to be called when a thread ends
	PIN_AddThreadFiniFunction(ThreadEnd, 0);

    #if RTN_STAT || RTN_INSTR
    // Register I_Routine to be called to instrument rtn
    RTN_AddInstrumentFunction(I_Routine, 0);
    #endif

    #if RTN_INSTR

    // Register I_PLTTrace to be called to instrument plt traces
  	TRACE_AddInstrumentFunction(I_PLTTrace, 0);

    #else

    // Register I_Trace to be called to instrument traces
    TRACE_AddInstrumentFunction(I_Trace, 0);

    #endif

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

