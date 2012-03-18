// =============================================================
// This tool computes the calling context tree
// =============================================================

// Authors:      Camil Demetrescu, Irene Finocchi
// Last changed: $Date: 2010/10/24 06:17:37 $
// Changed by:   $Author: demetres $
// Revision:     $Revision: 1.54 $

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include "STool.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// -------------------------------------------------------------
// Macros
// -------------------------------------------------------------

#define LOG2(x) ((int)(log(x)/log(2)))


// -------------------------------------------------------------
// Constants
// -------------------------------------------------------------

#define DEBUG           0       // if 1, trace analysis operations on stdout
#define DUMP_STAT       1       // if 1, compute and write statistics about the cct to file
#define DUMP_TREE       0       // if 1, dump to file the cct
#define TAB             4       // tab size for cct indentation
#define CHECK_SP        1       // if 1, check if a call always lets the stack grow as it should be
#define TRACK_SP        0       // if 1, include CPU stack pointer values in cct nodes

#define EMPTY_ANALYSIS  0       // if 1, analysis routines are empty (performance benchmark reference)

#ifndef CALL_SITE
#define CALL_SITE       0       // if 1, treat calls from different call sites as different
#endif


// -------------------------------------------------------------
// Structures
// -------------------------------------------------------------

// Record associated with CCT node
typedef struct CCTNode {
    struct CCTNode* firstChild;      // first child of the node in the CCT
    struct CCTNode* nextSibling;     // next sibling of the node in the CCT
    UINT64          count;           // number of times the context is reached 
    ADDRINT         target;          // address of call target within function associated with CCT node
    #if TRACK_SP
    UINT64          minStackPointer; // min stack pointer value for this node
    UINT64          maxStackPointer; // max stack pointer value for this node
    #endif
    #if CALL_SITE
    ADDRINT         callSite;        // address of calling instruction
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
    UINT32 freqSum[64]; // freqSum[i] = sum of call counts of nodes called x times, with x >= numCalls/2^{i}
} CCTStat;


// Call stack activation record
typedef struct ActivationRec {
    CCTNode* node;      // pointer to the CCT node associated with the call
} ActivationRec;


// Thread local data record
typedef struct ThreadRec {
    CCTNode* root;           // root of the CCT
    #if CHECK_SP
    ADDRINT minStackPointer; // minimum value of stack pointer for this thread
    ADDRINT maxStackPointer; // maximum value of stack pointer for this thread
    UINT32 stackErrors;      // number of times SP of caller <= SP of callee
                             // (the stack grows upside down)
    #endif
} ThreadRec;


// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

#if DUMP_STAT == 1
static UINT32 numRTN;             // total number of routines (except plt's)
static UINT32 numDirectCall;      // total number of instrumented direct calls made outside plt's
static UINT32 numIndirectCall;    // total number of instrumented indirect calls
#endif

// =============================================================
// Utility functions
// =============================================================

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
static UINT64 hotTreeSize(CCTNode* root, UINT64 threshold) {

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
// Get weight of hot calling context tree (HCCT), i.e. a subtree of the CCT
// obtained by pruning all non-hot nodes with no hot descendants.
// A node is in the HCCT if has a counter >= threshold or has a hot descendant
// -------------------------------------------------------------
static UINT64 hotTreeWeight(CCTNode* root, UINT64 threshold) {

    // skip empty subtrees
    if (root == NULL) return 0;

    // init accumulator
    UINT64 hotWeight = 0;

    // call recursively function on children
    for (CCTNode* theNodePtr = root->firstChild; theNodePtr != NULL; theNodePtr = theNodePtr->nextSibling)
        hotWeight += hotTreeWeight(theNodePtr, threshold);

    // if the root has a hot descendant or has a counter above the threshold, then count the root
    if (hotWeight > 0 || root->count >= threshold) return hotWeight + root->count;
    
    return hotWeight;
}


// -------------------------------------------------------------
// Build hot calling context tree (HCCT) - see hotTreeSize()
// -------------------------------------------------------------
static BOOL pruneTree(CCTNode* root, UINT64 threshold) {

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
    #if CALL_SITE
    fprintf(out, "%s - %s [%x->%x] (%llu)", 
            STool_RoutineName(root->target), STool_StripPath(STool_LibraryName(root->target)), 
            root->callSite, root->target, root->count);
    #else
    fprintf(out, "%s - %s [%x] (%llu)", 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target, root->count);
    #endif

    #if TRACK_SP
    fprintf(out, "  [minSP=%llu, maxSP=%llu]", 
        root->minStackPointer, root->maxStackPointer);
    if (root->minStackPointer != root->maxStackPointer)
        fprintf(out, " **** different stack pointers for same context");
    for (CCTNode* theNodePtr = root->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling) 
         if (root->minStackPointer <= theNodePtr->minStackPointer) {
            fprintf(out, " #### WARNING: child with same stack depth");
            break;
         }
    #endif

    fprintf(out, "\n");

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
#if DUMP_STAT == 1
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
            if (root->count >= N) {
                stat->freqDis[i]++;
                stat->freqSum[i]+=root->count;
            }

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
#endif


// -------------------------------------------------------------
// Write output to file
// -------------------------------------------------------------
VOID WriteOutput(ThreadRec* tdata, FILE* out) {

    #if DUMP_STAT
    CCTStat stat;

    // compute CCT statistics
    getStat(tdata->root, &stat);

    // dump CCT statistics
    fprintf(out, "----------\nApplication: ");
    fprintf(out, "%s ", STool_CommandLine());
    fprintf(out, "- Thread ID: %u - CALL_SITE = %d\n\n", STool_ThreadID(tdata), CALL_SITE);

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

    #if CHECK_SP
    fprintf(out, "\nMinimum stack pointer value: %lu (%p)\n", (unsigned long)tdata->minStackPointer, (void*)tdata->minStackPointer);
    fprintf(out, "Maximum stack pointer value: %lu (%p)\n", (unsigned long)tdata->maxStackPointer, (void*)tdata->maxStackPointer);
    fprintf(out, "Max stack size: %lu bytes\n", (unsigned long)tdata->maxStackPointer-(unsigned long)tdata->minStackPointer);
    fprintf(out, "Number of stack errors (SP of caller <= SP of callee): %lu\n", (unsigned long)tdata->stackErrors);
    #endif

    // find max j for which stat.callDis[j] > 0
    int j;
    for (j=31; j>=0 && !stat.callDis[j]; j--);

    fprintf(out, "\nDistribution of CCT node calls - aggregate (cct nodes accounting for given fractions of calls):\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s %-14s %-14s %-14s %-15s %-15s %-15s\n", 
                 "i", "% nodes", "% calls", "# nodes", "# calls", "min node freq", "(% of calls)", "# HCCT nodes", "# cold in HCCT", "% cold in HCCT", "space saving %");
    UINT64 callSum = 0;
    UINT64 nodeSum = 0;
    for (int i=j; i>=0; i--) {
        UINT64 hcctSize = hotTreeSize(tdata->root, 1<<i);
        callSum += stat.callAgg[i];
        nodeSum += stat.callDis[i];
        fprintf(out, "%-4d %-14.2f %-14.2f %-14llu %-14llu %-14u %-14.6f %-14llu %-15llu %-15.2f %-15.2f\n", 
            i, 
            100.0*nodeSum/stat.numNodes, 
            100.0*callSum/stat.numCalls, 
            nodeSum, 
            callSum, 
            1<<i, 
            100.0*(1<<i)/stat.numCalls, 
            hcctSize, 
            hcctSize - nodeSum, 
            100.0*(hcctSize - nodeSum)/hcctSize, 
            100.0*(stat.numNodes-hcctSize)/stat.numNodes);
    }

    fprintf(out, "\nDistribution of CCT node calls as a fraction of the total number of calls N:\n\n");
    fprintf(out, "%-4s %-22s %-14s %-14s %-14s %-11s %-14s %-14s %-14s %14s %18s\n", 
                 "i", 
                 "threshold t %", 
                 "tN", 
                 "# nodes >= tN", 
                 "% nodes >= tN", 
                 "HCCT size",
                 "1/t",
                 "1/e (e=t/10)", 
                 "CCT overlap %",
                 "space saving %",
                 "1/e space saving %");
    for (int i=0; i<64 && (i==0 || stat.freqDis[i-1]<stat.numNodes); ++i) {
        UINT64 hcctSize = hotTreeSize(tdata->root, stat.numCalls/((UINT64)1 << i));
        UINT64 oneOverT = ((UINT64)1 << i) > stat.numNodes ? stat.numNodes : ((UINT64)1 << i);
        UINT64 oneOverE = 10*((UINT64)1 << i) > stat.numNodes ? stat.numNodes : 10*((UINT64)1 << i);
        fprintf(out, "%-4d %22.17f %-14llu %-14u %-14.3f %-11llu %-14llu %-14llu %-14.3f %-14.3f %-18.3f\n", 
            i, 
            100.0/(double)((UINT64)1 << i), 
            stat.numCalls/((UINT64)1 << i), 
            stat.freqDis[i], 
            (stat.numNodes) > 0 ? 100.0*stat.freqDis[i]/(stat.numNodes) : 0.0,
            hcctSize,
            oneOverT, 
            oneOverE,
            100.0*hotTreeWeight(tdata->root, stat.numCalls/((UINT64)1 << i))/stat.numCalls,
            (stat.numNodes) > 0 ? 100.0-100.0*hcctSize/stat.numNodes : 0.0,
            (stat.numNodes) > 0 ? 100.0-100.0*oneOverE/stat.numNodes : 0.0);
    }

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
    dumpTree(tdata->root, 0, out);
    #endif
}


// =============================================================
// Callback functions
// =============================================================

// -------------------------------------------------------------
// Thread start function
// -------------------------------------------------------------
VOID ThreadStart(ThreadRec* tdata) {

    #if DEBUG
    printf("[TID=%u] Starting new thread\n", STool_ThreadID(tdata));
    #endif

    // allocate dummy CCT root
    tdata->root = (CCTNode*)calloc(sizeof(CCTNode), 1);
    if (tdata->root == NULL) exit((printf("[TID=%u] Can't allocate CCT root node\n", STool_ThreadID(tdata)),1));
    tdata->root->count = 1;

    #if CHECK_SP
    tdata->maxStackPointer = 0;
    tdata->minStackPointer = 0xFFFFFFFF;
    #endif
}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
VOID ThreadEnd(ThreadRec* tdata) {

    FILE* out;
    char fileName[32];

    // make output file name
    #if CALL_SITE
    sprintf(fileName, "cct-%s-%u.cs.out", STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
    #else
    sprintf(fileName, "cct-%s-%u.out", STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
    #endif

    // print message
    printf("[%s] Creating output file...", fileName);

    // create output file
    out = fopen(fileName, "w");
    if (out == NULL) exit((printf("Can't create output file %s\n", fileName), 1));

    // write output file
    WriteOutput(tdata, out);

    // close output file
    fclose(out);

    // deallocate CCT
    freeTree(tdata->root);

    // print message
    printf(" done.\n");
}


// =============================================================
// Analysis functions
// =============================================================

// -------------------------------------------------------------
// Function enter event
// -------------------------------------------------------------
void RoutineEnter(ThreadRec* tdata
                  #if CALL_SITE
                  , ADDRINT ip
                  #endif
                  ) {

    #if EMPTY_ANALYSIS == 0
    
    ADDRINT target = STool_CurrRoutine(tdata);

    #if TRACK_SP || CHECK_SP
    ADDRINT stackPointer = STool_CurrStackPointer(tdata);
    #endif
    
    #if CHECK_SP
    if (stackPointer < tdata->minStackPointer)
        tdata->minStackPointer = stackPointer;
    if (stackPointer > tdata->maxStackPointer)
        tdata->maxStackPointer = stackPointer;
    if (STool_NumActivations(tdata) > 1 && 
        stackPointer >= STool_StackPointerAt(tdata, STool_NumActivations(tdata)-2))
        tdata->stackErrors++;
    #endif

    CCTNode* theNodePtr;
    CCTNode* theParentPtr = STool_NumActivations(tdata) > 1 ? 
                            STool_ActivationAt(tdata, STool_NumActivations(tdata)-2, ActivationRec).node : 
                            tdata->root;    

    // did we already encounter this context?
    for (theNodePtr = theParentPtr->firstChild;
         theNodePtr != NULL;
         theNodePtr = theNodePtr->nextSibling)
            #if CALL_SITE
            if (theNodePtr->callSite == ip && theNodePtr->target == target) goto Exit;
            #else
            if (theNodePtr->target == target) goto Exit;
            #endif

    // create new context node
    theNodePtr = (CCTNode*)calloc(sizeof(CCTNode), 1);
    if (theNodePtr == NULL) exit((printf("[TID=%u] Out of memory error\n", STool_ThreadID(tdata)),1));
    
    // init node and add it to tree
    theNodePtr->nextSibling = theParentPtr->firstChild;
    theParentPtr->firstChild = theNodePtr;
    theNodePtr->target = target;
    #if CALL_SITE
    theNodePtr->callSite = ip;
    #endif
    
    #if TRACK_SP
    theNodePtr->minStackPointer = theNodePtr->maxStackPointer = stackPointer;
    #endif

  Exit:

    // store context node into current activation record
    STool_CurrActivation(tdata, ActivationRec).node = theNodePtr;

    // increase context counter
    theNodePtr->count++;

    #if TRACK_SP
    if (stackPointer < theNodePtr->minStackPointer)
        theNodePtr->minStackPointer = stackPointer;
    if (stackPointer > theNodePtr->maxStackPointer)
        theNodePtr->maxStackPointer = stackPointer;
    #endif
    
    #endif
}


// =============================================================
// Main function of the CCT analysis tool
// =============================================================

int main(int argc, char * argv[]) {
    
    STool_TSetup setup = { 0 };

    setup.argc              = argc;
    setup.argv              = argv;
    setup.callingSite       = CALL_SITE;
    setup.activationRecSize = sizeof(ActivationRec);
    setup.threadRecSize     = sizeof(ThreadRec);
    setup.threadStart       = (STool_TThreadH)ThreadStart;
    setup.threadEnd         = (STool_TThreadH)ThreadEnd;
    setup.rtnEnter          = (STool_TRtnH)RoutineEnter;
    
    // run application and analysis tool
    STool_Run(&setup);

    return 0;
}

