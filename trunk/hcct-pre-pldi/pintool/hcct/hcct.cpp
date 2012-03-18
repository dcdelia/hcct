// =============================================================
// This tool computes the calling context tree
// =============================================================

// -------------------------------------------------------------
// Includes
// -------------------------------------------------------------

#include "STool.h"
#include "streaming.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
#include <fcntl.h>

// -------------------------------------------------------------
// Macros
// -------------------------------------------------------------

#define LOG2(x) ((int)(log(x)/log(2)))

// -------------------------------------------------------------
// Constants
// -------------------------------------------------------------

#define DEBUG           0       // if 1, trace analysis operations on stdout
#define TAB             4       // tab size for cct indentation

#define EMPTY_ANALYSIS  0       // if 1, instrumentation analysis routines are empty (performance benchmark reference)

// -------------------------------------------------------------
// Structures
// -------------------------------------------------------------

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
} ActivationRec;

// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------

static UINT32 numRTN;             // total number of routines (except plt's)
static UINT32 numDirectCall;      // total number of instrumented direct calls made outside plt's
static UINT32 numIndirectCall;    // total number of instrumented indirect calls

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
// Dump calling context tree (DIMACS format)
// -------------------------------------------------------------
#if DUMP_TREE
static VOID dumpNodes(CCTNode* root, FILE* out) {
	// postorder traversal for nodes
	CCTNode* child;
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)
		dumpNodes(child, out);
	#if STREAMING
	ST_dumpNode(root, out);
	#else
	#if CALL_SITE
    fprintf(out, "v %lu %s %s [%x->%x] %d\n", (unsigned long)root, 
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->callSite, root->target, root->count);
    #else
    fprintf(out, "v %lu %s %s [%x] %d \n", (unsigned long)root,
            STool_RoutineNameByAddr(root->target), STool_StripPath(STool_LibraryNameByAddr(root->target)), 
            root->target, root->count);
    #endif
	#endif
}

static VOID dumpEdges(CCTNode* node, FILE* out) {
	// inorder traversal for edges (parent->child)
	fprintf(out, "a %lu %lu\n", (unsigned long)node->parent, (unsigned long)node);
	CCTNode* child;
	for (child=node->firstChild; child!=NULL; child=child->nextSibling)
		dumpEdges(child, out);
}

static VOID dumpTree(ThreadRec* tdata, FILE* out) {
	#if STREAMING
	fprintf(out, "p %d %ld\n", ST_nodes(tdata), ST_length(tdata));
	#else
	fprintf(out, "p %d %ld\n", tdata->numNodes, tdata->numCalls); // FIX HERE
	#endif
	dumpNodes(tdata->root,out);
	CCTNode* child;
	for (child=tdata->root->firstChild; child!=NULL; child=child->nextSibling)
		dumpEdges(child,out);
}
#endif

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
#if DUMP_STAT
VOID WriteOutput(ThreadRec* tdata, FILE* out) {

    CCTStat stat;

    // compute CCT statistics
    getStat(tdata->root, &stat);

    // dump CCT statistics
    fprintf(out, "----------\nApplication: ");
    fprintf(out, "%s ", STool_CommandLine());
    fprintf(out, "- Thread ID: %u - CALL_SITE = %d\n\n", STool_ThreadID(tdata), CALL_SITE);

	// Thread time
	timespec threadTime;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &threadTime);
	struct tm* t;
	t=localtime(&threadTime.tv_sec);
	//fprintf(out, "- Thread time: %d h %d m %d s %ld nsec\n", t->tm_hour-1, t->tm_min, t->tm_sec, threadTime.tv_nsec);	
	fprintf(out, "Thread time: %d h %d m %d s\n", t->tm_hour-1, t->tm_min, t->tm_sec);
	//fprintf(out, "Test: %s\n", ctime(&threadTime.tv_sec));

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
        UINT64 hcctSize = hotTreeSize(tdata->root ,1<<i);
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
}
#endif

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
    tdata->root = (CCTNode*)calloc(sizeof(CCTNode)+fatNodeBytes, 1); // fat node
    if (tdata->root == NULL) exit((printf("[TID=%u] Can't allocate CCT root node\n", STool_ThreadID(tdata)),1));
    tdata->root->count = 1;
	tdata->root->parent=NULL; // ???

	#if STREAMING
	// streaming stuff
	ST_new(tdata);
	#else
	tdata->numNodes=1;
	tdata->numCalls=1;
	#endif
}


// -------------------------------------------------------------
// Thread end function
// -------------------------------------------------------------
VOID ThreadEnd(ThreadRec* tdata) {
	int ds;
	srand(time(NULL));
	int random=rand();
    
	#if STREAMING && ST_DUMP
	// dump streaming algorithm's data

    FILE*  dump;
	int ds3;
    char dumpFileName[64]; 

	#if CALLING_SITE    
		sprintf(dumpFileName, "hcct-%s-%u.cs.stdata",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
	#else
		sprintf(dumpFileName, "hcct-%s-%u.stdata",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
	#endif
	
	ds3 = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
	if (ds3 == -1) {
		#if CALLING_SITE    
		sprintf(dumpFileName, "hcct-%s-%u.cs.stdata.%d",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random);
		#else
		sprintf(dumpFileName, "hcct-%s-%u.stdata.%d",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random);
		#endif
		ds3 = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
		if (ds3 == -1) exit((printf("can't create output file %s\n", dumpFileName), 1));
	}
		
    dump = fdopen(ds3, "w");
    if (dump == NULL) exit((printf("can't open output file %s\n", dumpFileName), 1));

	// print message
    printf("[%s] Creating output file...\n", dumpFileName);
	
	ST_dumpStats(tdata, dump);

	fclose(dump);
	#endif

	// dump tree
	#if DUMP_TREE
	int ds2;
	FILE* dumpTreeFile;
	char dumpTreeFileName[64]; 
	
	#if CALLING_SITE    
		sprintf(dumpTreeFileName, "hcct-%s-%u.cs.dump",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
	#else
		sprintf(dumpTreeFileName, "hcct-%s-%u.dump",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
	#endif

	ds2 = open(dumpTreeFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
	if (ds2 == -1) {
		#if CALLING_SITE    
		sprintf(dumpTreeFileName, "hcct-%s-%u.cs.dump.%d",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random);
		#else
		sprintf(dumpTreeFileName, "hcct-%s-%u.dump.%d",  STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random);
		#endif
		ds2 = open(dumpTreeFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
		if (ds2 == -1) exit((printf("can't create output file %s\n", dumpTreeFileName), 1));
	}
	
    dumpTreeFile = fdopen(ds2, "w");
    if (dumpTreeFile == NULL) exit((printf("can't open output file %s\n", dumpTreeFileName), 1));

	// print message
    printf("[%s] Creating output file...\n", dumpTreeFileName);

	dumpTree(tdata, dumpTreeFile);

	fclose(dumpTreeFile);
	#endif

	// Create output file with tree statistics
	#if DUMP_STAT
	FILE* out;
    char fileName[64];

    // make output file name
    #if CALL_SITE
    sprintf(fileName, "cct-%s-%u.cs.out", STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
    #else
    sprintf(fileName, "cct-%s-%u.out", STool_StripPath(STool_AppName()), STool_ThreadID(tdata));
    #endif

    // create output file
	ds = open(fileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
	if (ds == -1) {
		// file already exists => choose random suffix
		#if CALL_SITE
		sprintf(fileName, "cct-%s-%u.cs.out.%d", STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random;
		#else
		sprintf(fileName, "cct-%s-%u.out.%d", STool_StripPath(STool_AppName()), STool_ThreadID(tdata), random);
		#endif
		ds = open(fileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
		if (ds == -1) exit((printf("Can't create output file %s\n", fileName), 1));
	}
    out = fdopen(ds, "w");
    if (out == NULL) exit((printf("Can't open output file %s\n", fileName), 1));
	
	// print message
    printf("[%s] Creating output file...\n", fileName);

    // write output file
    WriteOutput(tdata, out);

    // close output file
    fclose(out);
	#endif

	#if STREAMING
	// deallocate streaming algorithm's data
	ST_free(tdata);
	#endif

	// deallocate CCT
    freeTree(tdata->root);
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
    theNodePtr = (CCTNode*)calloc(sizeof(CCTNode)+fatNodeBytes, 1);
    if (theNodePtr == NULL) exit((printf("[TID=%u] Out of memory error\n", STool_ThreadID(tdata)),1));
	#if STREAMING==0
	tdata->numNodes++;
	#endif
    
    // add node to tree
	theNodePtr->parent=theParentPtr;
    theNodePtr->nextSibling = theParentPtr->firstChild;
    theParentPtr->firstChild = theNodePtr;
    theNodePtr->target = target;
    #if CALL_SITE
    theNodePtr->callSite = ip;
    #endif

  Exit:

    // store context node into current activation record
    STool_CurrActivation(tdata, ActivationRec).node = theNodePtr;

    // increase context counter
    theNodePtr->count++;
    
	#if STREAMING
	// streaming
	ST_update(tdata, theNodePtr);
	#else
	tdata->numCalls++;
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
    setup.threadRecSize     = sizeof(ThreadRec)+fatThreadBytes;
    setup.threadStart       = (STool_TThreadH)ThreadStart;
    setup.threadEnd         = (STool_TThreadH)ThreadEnd;
    setup.rtnEnter          = (STool_TRtnH)RoutineEnter;

    // run application and analysis tool
	
    STool_Run(&setup);

    return 0;
}

