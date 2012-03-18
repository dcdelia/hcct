// =============================================================
// This tool computes the hot calling context tree
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
#define DUMP_TREE       1       // if 1, dump to file the cct
#define TAB             4       // tab size for cct indentation
#define RTN_INSTR       0       // if 1, instrument calls/rts at RTN level (but plt exits, made at TRACE LEVEL)
                                // if 0, instrument calls/rts at TRACE level
#define RTN_STAT        0       // if 1 or RTN_INSTR==1, compute routine statistics
#define EMPTY_ANALYSIS  0       // if 1, instrumentation analysis routines are empty (performance benchmark reference)

#ifndef RESOLVE_TARGET
#define RESOLVE_TARGET  1       // if 1, let targets of calls always point to the routine start address
#endif

#ifndef CALLING_SITE
#define CALLING_SITE    0       // if 1, treat calls from different calling sites as different
#endif

#ifndef USE_COUNTCHILD
#define USE_COUNTCHILD  1       // metric (count: 0, countChild: 1)
#endif

#ifndef CORRECT_HCCT
#define CORRECT_HCCT	0		// if 0, do batch "continuous" pruning	
#endif

#define PRUNE_BY_CALLS  0
#define PRUNE_BY_NODES  1

#define USE_MALLOC      0       // if 1, use default malloc()/free()

// -------------------------------------------------------------
// Structures
// -------------------------------------------------------------

// Record associated with CCT node
typedef struct CCTNode {
    struct CCTNode* firstChild;   // first child of the node in the CCT
    struct CCTNode* nextSibling;  // next sibling of the node in the CCT
	struct CCTNode* parent;		  // updates and remove
    UINT64          count;        // number of times the context is reached 
    #if USE_COUNTCHILD
	UINT64			countChild;
	UINT64			update;
    #endif
    UINT64          error;
	UINT64			errorChild;
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
} CCTStat;

// Call stack activation record
typedef struct ActivationRec {
    CCTNode* node;      // pointer to the CCT node associated with the call
    ADDRINT  currentSP; // stack pointer register content before the call
} ActivationRec;

// Prune data record
typedef struct PruneData {
    CCTNode*    nodeSP;
    int       	thresholdN;
	int			thresholdD;
    UINT64      calls;
    UINT64      nodes;
    UINT64      growth; // growth of nodes between two prunings
    PruneData*  prev;
	#if !USE_MALLOC
    CCTNode*       freeList;
    #endif
} PruneData;

// Thread local data record
typedef struct ThreadRec {
    ActivationRec* callStack;
    UINT32         callStackTop;
    UINT32         callStackSize;
    THREADID       threadID;
    UINT8          pad[PAD_SIZE];   // to avoid false sharing problems
	UINT64		   numNodes;
	UINT64		   numCalls;
    PruneData*     pdata;
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
// Alternative malloc/free scheme
// -------------------------------------------------------------
#if !USE_MALLOC
CCTNode* a_malloc(PruneData* pdata) {
    if (pdata->freeList->firstChild==NULL)
//        return (CCTNode*)malloc(sizeof(CCTNode));
		return (CCTNode*)calloc(sizeof(CCTNode), 1);
    else {
		CCTNode* res=pdata->freeList->firstChild;
		pdata->freeList->firstChild=res->firstChild;
		//res->parent=NULL;
		//res->firstChild=NULL; // HERE
		//res->nextSibling=NULL;
		return res; // Il nodo restituito è sporco, ma lo inizializzo nel chiamante come per le malloc :)
	}
}

void a_free(CCTNode* node, PruneData* pdata) {
	node->firstChild=pdata->freeList->firstChild;
	pdata->freeList->firstChild=node;
}
#endif

// -------------------------------------------------------------
// Print help message                                                
// -------------------------------------------------------------
static INT32 Usage() {
    fprintf(stderr, "This Pintool creates the hot calling context tree of a running program\n");
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
  if (name == "") return *new string("<unknown_routine>");
  else return *new string(name);
}


// -------------------------------------------------------------
// Get library name from address
// -------------------------------------------------------------
const string& Target2LibName(ADDRINT target) {
    PIN_LockClient();
    
    const RTN rtn = RTN_FindByAddress(target);
    static const string _invalid_rtn("<unknown_image>");
    string name;
    
    if( RTN_Valid(rtn) ) name = IMG_Name(SEC_Img(RTN_Sec(rtn)));
    else name = _invalid_rtn;

    PIN_UnlockClient();
    return *new string(name);
}

// test

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
// Dump calling context tree
// -------------------------------------------------------------
//~ static VOID dumpTree(CCTNode* root, UINT32 indent, FILE* out) {

    //~ // skip empty subtrees
    //~ if (root == NULL) return;

    //~ // indent line
    //~ for (UINT32 i=0; i<indent; ++i) 
        //~ if (i + TAB < indent) fprintf(out, (i % TAB) ? " " : "|");
        //~ else fprintf(out, (i % TAB) ? ( (i < indent - 1 ) ? " " : ( i < indent ? " " : " ")) : "|");
        
    //~ // print function name and context count
    //~ #if USE_COUNTCHILD
    //~ fprintf(out, "%s (%llu), MAX ERROR: (%llu), CC: (%llu)\n", Target2RtnName(root->target).c_str(), root->count, root->error, root->countChild);
    //~ #else
    //~ fprintf(out, "%s (%llu), MAX ERROR: (%llu)\n", Target2RtnName(root->target).c_str(), root->count, root->error);
    //~ #endif
    //~ // call recursively function on children
    //~ for (CCTNode* theNodePtr = root->firstChild;
         //~ theNodePtr != NULL;
         //~ theNodePtr = theNodePtr->nextSibling) dumpTree(theNodePtr, indent + TAB, out);
//~ }

static VOID dumpNodes(CCTNode* root, FILE* out) {
	// Visita in postordine per facilitare la creazione dei nodi in fase di analisi
	CCTNode* child;
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)
		dumpNodes(child, out);
	// fprintf(out, "v %lu %d %lu\n", (long)root, root->target, root->count);
	fprintf(out, "v %lu %s %lu\n", (unsigned long)root, Target2RtnName(root->target).c_str(), root->count);	
}

static VOID dumpEdges(CCTNode* node, FILE* out) {
	// Visita in ordine per facilitare l'inserimento degli archi in fase di analisi
	// Inseriamo gli archi come se fossero padre->figlio, ma accedendovi mediante parent non posso partire da root!
	fprintf(out, "a %lu %lu\n", (long)node->parent, (long)node);
	CCTNode* child;
	for (child=node->firstChild; child!=NULL; child=child->nextSibling)
		dumpEdges(child, out);
}

static VOID dumpTree(CCTNode* root, FILE* out, ThreadRec* tdata) {
	// passo tdata e non pdata perché potrebbero servire nuove informazioni in futuro
	tdata->pdata->nodes++;
	fprintf(out, "n %llu\n", tdata->pdata->nodes);
	dumpNodes(root,out);
	CCTNode* child;
	for (child=root->firstChild; child!=NULL; child=child->nextSibling)
		dumpEdges(child,out);
}
// -------------------------------------------------------------
// Compute statistics of calling context tree
// -------------------------------------------------------------
static VOID getStat(CCTNode* root, CCTStat* stat) {

    // reset statistics
    memset(stat, 0, sizeof(CCTStat));

    // empty tree: skip
    if (root == NULL) return;

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

    push(stack, root, 1);

    while (stack != NULL) {

        UINT32 degr = 0, depth;
    
        pop(stack, root, depth);

        // recursively scan subtrees
        for (CCTNode* child = root->firstChild;
             child != NULL;
             child = child->nextSibling) {

             degr++;
             push(stack, child, depth + 1);
        }

        // count nodes and total calls
        stat->numNodes++;
        stat->numCalls += root->count;
        
        // compute distribution of call counts of nodes 
        // callDis[i] = # of CCT nodes that were reached x times, with x in [2^i, 2^(i+1))
        stat->callDis[LOG2(root->count)]++;
        stat->callAgg[LOG2(root->count)] += root->count;

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
// Pruning stuff
// -------------------------------------------------------------
#if USE_MALLOC
VOID removeChild(CCTNode* node) {
#else
VOID removeChild(CCTNode* node, PruneData* pdata) {
#endif
	CCTNode* pfs=node->parent->firstChild;
	if (node==pfs) {
		node->parent->firstChild=node->nextSibling;
		free(node);
	} else {
		while (pfs->nextSibling!=NULL)
			if (pfs->nextSibling==node) {
				pfs->nextSibling=pfs->nextSibling->nextSibling;
                #if USE_MALLOC
				free(node); // i figli saranno stati rimossi prima :) (soglia fissa)
				#else
				a_free(node, pdata);
                #endif
				break;
			} else pfs=pfs->nextSibling;
	}
}

#if USE_COUNTCHILD
void finalPrune(CCTNode* node, PruneData* pdata) {
    CCTNode *child, *next;
	for (child=node->firstChild; child!=NULL;) {
        next=child->nextSibling; // nel caso di eliminazioni
		finalPrune(child, pdata);
        child=next;
	}
    node->parent->countChild+=node->update;
    node->parent->update+=node->update;
    node->update=0; // reset
    if ((node->count + node->countChild) < ((pdata->prev->thresholdN*pdata->prev->calls)/pdata->prev->thresholdD)) {
		// FIX (see below)
		//node->parent->errorChild+=node->count-node->error; // update errChild
		
		// NON SERVE!!!
		//node->parent->errorChild+=node->count - node->error + node->errorChild; // peso l'errore sul sottoalbero!!!
        
        pdata->nodes--;

		#if USE_MALLOC
        removeChild(node);
		#else
		removeChild(node, pdata);
		#endif
    }
}
#else
void finalPrune(CCTNode* node, PruneData* pdata, ThreadRec* tdata) {
    CCTNode *child, *next;
	for (child=node->firstChild; child!=NULL;) {
        next=child->nextSibling; // nel caso di eliminazioni
		finalPrune(child, pdata, tdata);
        child=next;
	}
    // Devo proteggere i figli
    if (node->firstChild==NULL && node->count < ((pdata->prev->thresholdN*pdata->prev->calls)/pdata->prev->thresholdD)) {
		
		// NON SERVE	
		//node->parent->errorChild+=node->count-node->error; // update errChild
		
        pdata->nodes--;
        
        if (node!=tdata->callStack[0].node) // devo proteggere il primo nodo (vedi statistiche)
			#if USE_MALLOC
            removeChild(node);
			#else
			removeChild(node,pdata);
			#endif
    }
}
#endif

int Prune(CCTNode* node, PruneData* pdata) {
    int inUse=0;
    CCTNode *child, *next;
	for (child=node->firstChild; child!=NULL;) {
        next=child->nextSibling; // nel caso di eliminazioni
		inUse+=Prune(child, pdata);
        child=next;
	}
    #if USE_COUNTCHILD
    node->parent->countChild+=node->update;
	node->parent->update+=node->update;
	node->update=0; // FIXED
    #endif

	#if !CORRECT_HCCT
	// CORRECT_HCCT=1 ==> only "final" pruning
    if (inUse>0 || node==pdata->nodeSP) // a child is in use OR this node is in use
        return 1;
    CCTNode* tmp=node; // boh
    for (tmp=pdata->nodeSP->parent; tmp!=NULL; tmp=tmp->parent)
        if (node==tmp) return 1; // il problema non è qui!!!! :O
    #if USE_COUNTCHILD
	// Fattore di correzione??? Proviamo 50 (test su vlc)
    if (50*(node->count + node->countChild) < ((pdata->prev->thresholdN * pdata->prev->calls) / pdata->prev->thresholdD)) {
    #else
    // Devo proteggere i figli (lo taglio eventualmente solo se è una foglia!)
    if (node->firstChild==NULL && node->count < ((pdata->prev->thresholdN * pdata->prev->calls) / pdata->prev->thresholdD)) {
    #endif
        // STIMA "ESATTA"
		node->parent->errorChild+=(node->count - node->error)+(node->errorChild - node->error);
		// TEST
		//node->parent->errorChild+=(node->count - node->error);
        pdata->nodes--;
        #if USE_MALLOC
		removeChild(node);
		#else
		removeChild(node,pdata);
		#endif
    }
	#endif
    return 0;
}

// -------------------------------------------------------------
// Write output to file
// -------------------------------------------------------------

VOID writeStats(CCTStat* pstat, FILE* out, ThreadRec* tdata) {
    CCTStat stat=*pstat; // :(
    fprintf(out, "----------\nApplication: ");
    for (int i=0; i<ArgC-1;) 
        if (!strcmp("--", ArgV[i++])) 
             for (; i<ArgC; i++) fprintf(out, "%s ", ArgV[i]);
    fprintf(out, "- Thread ID: %u - RESOLVE_TARGET = %d - CALLING_SITE = %d\n\n", tdata->threadID, RESOLVE_TARGET, CALLING_SITE);

    if (numRTN) fprintf(out, "Number of routines: %u\n", numRTN);
    fprintf(out, "Number of calls: %llu\n", stat.numCalls);
    fprintf(out, "Number of nodes of the HCCT: %llu\n", stat.numNodes);
    fprintf(out, "Number of leaves of the HCCT: %llu (%.0f%%)\n", stat.numLeaves, 100.0*stat.numLeaves/stat.numNodes);
    fprintf(out, "Number of internal nodes of the HCCT: %llu (%.0f%%)\n", stat.numNodes-stat.numLeaves, 100.0*(stat.numNodes-stat.numLeaves)/stat.numNodes);
    if (numRTN) {
        fprintf(out, "\nAverage number of direct calls per routine: %.1f\n", (double)numDirectCall/numRTN);
        fprintf(out, "Average number of indirect calls per routine: %.1f\n", (double)numIndirectCall/numRTN);
    }

    fprintf(out, "\nHCCT maximum depth: %u\n",  stat.maxDepth);
    fprintf(out, "HCCT average depth of a leaf: %.0f\n", stat.numLeaves > 0 ? (double)stat.totDepth/stat.numLeaves : 0.0);
    fprintf(out, "HCCT maximum degree: %u\n", stat.maxDegr);
    fprintf(out, "HCCT average degree of an internal node: %.2f\n", (stat.numNodes-stat.numLeaves) > 0 ? (double)stat.totDegr/(stat.numNodes-stat.numLeaves) : 0.0);
    fprintf(out, "HCCT average weighted degree of an internal node: %.2f\n", (stat.numIntCalls) > 0 ? (double)stat.totWDegr/stat.numIntCalls : 0.0);

    // find max j for which stat.callDis[j] > 0
    int j;
    for (j=31; j>=0 && !stat.callDis[j]; j--);

    fprintf(out, "\nDistribution of HCCT node calls - aggregate (HCCT nodes accounting for given fractions of calls):\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s %-14s %-14s\n", 
                 "i", "% nodes", "% calls", "# nodes", "# calls", "min node freq", "(% of calls)");
    UINT64 callSum = 0;
    UINT64 nodeSum = 0;
    for (int i=j; i>=0; i--) {
        callSum += stat.callAgg[i];
        nodeSum += stat.callDis[i];
        fprintf(out, "%-4d %-14.2f %-14.2f %-14llu %-14llu %-14u %-14.6f\n", 
            i, 100.0*nodeSum/stat.numNodes, 100.0*callSum/stat.numCalls, 
            nodeSum, callSum, 1<<i, 100.0*(1<<i)/stat.numCalls);
    }

    fprintf(out, "\nDistribution of HCCT node calls:\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s\n", 
                 "i", "calls from", "calls to", "# nodes", "% nodes");
    for (int i=0; i<=j; ++i)
        fprintf(out, "%-4d %-14u %-14u %-14u %-14.1f\n", 
            i, 1<<i, (1<<(i+1))-1, stat.callDis[i], 
            (stat.numNodes) > 0 ? 100.0*stat.callDis[i]/(stat.numNodes) : 0.0);

    fprintf(out, "\nDistribution of HCCT node degrees:\n\n");
    fprintf(out, "%-4s %-14s %-14s %-14s %-14s\n", 
                 "i", "degree from", "degree to", "# int. nodes", "% int. nodes");
    for (j=31; j>=0 && !stat.degrDis[j]; j--);
    for (int i=0; i<=j; ++i)
        fprintf(out, "%-4d %-14u %-14u %-14u %-14.1f\n", 
            i, 1<<i, (1<<(i+1))-1, stat.degrDis[i],
            (stat.numNodes-stat.numLeaves) > 0 ? 100.0*stat.degrDis[i]/(stat.numNodes-stat.numLeaves) : 0.0);

    // Statistics about growth of nodes and calls between two prunings
    PruneData* p;
    int x=0;
    fprintf(out, "\nStatistics about growth of nodes and #calls between two prunings (reverse order)\n---\n");
    for (p=tdata->pdata->prev; p->prev!=NULL; p=p->prev)
        fprintf(out, "Iteration: %d, growth: %llu, #nodes: %llu, #calls: %llu\n", x++, p->growth, p->nodes, p->growth);
}

VOID WriteOutput(ThreadRec* tdata, FILE* out) {
    // Preparing pruning
    PruneData* pdata=(PruneData*)malloc(sizeof(PruneData));
	#if !USE_MALLOC
    pdata->freeList=tdata->pdata->freeList;
	#endif
	pdata->growth=0; // statistics
    pdata->prev=tdata->pdata;
    tdata->pdata=pdata;
    pdata->thresholdN=pdata->prev->thresholdN;
	pdata->thresholdD=pdata->prev->thresholdD;
	pdata->nodes=pdata->prev->nodes;
    pdata->calls=tdata->numCalls;
    pdata->nodeSP=tdata->callStack[tdata->callStackTop].node;
    #if USE_COUNTCHILD
    finalPrune(tdata->callStack[0].node, pdata); // no protection needed
    #else
    finalPrune(tdata->callStack[0].node, pdata, tdata);
    #endif

	#if !USE_MALLOC
	// No memory leaks :)
	CCTNode *x, *y;
	for (x=pdata->freeList; x!=NULL; x=y) {
		y=x->firstChild;
		free(x);
	}
	#endif

    int countPrune=-1; // adjust
    PruneData* p;
    for (p=pdata; p->prev!=NULL; p=p->prev)
        countPrune++;

    #if DUMP_STAT
    CCTStat prunedStat;

    fprintf(out, "\n-------------------------------------------------------------\n");
    fprintf(out, "TREE HAS BEEN PRUNED %d TIMES AT %f\n", countPrune, pdata->thresholdN/(double)pdata->thresholdD);
    fprintf(out, "\nStatistics - #nodes: %llu, #calls: %llu\n", tdata->numNodes, tdata->numCalls);
    #if USE_COUNTCHILD
    fprintf(out, "Metric in use: countChild\n");
    #else
    fprintf(out, "Metric in use: count\n");
    #endif
    fprintf(out, "-------------------------------------------------------------\n\n");
    
    // compute CCT statistics
    getStat(tdata->callStack[0].node, &prunedStat);

    // dump CCT statistics
    writeStats(&prunedStat, out, tdata);
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
	
    // prune data
    PruneData* pdata=(PruneData*)malloc(sizeof(PruneData));
    PruneData* dummypdata=(PruneData*)malloc(sizeof(PruneData));
    //pdata->nodeSP=NULL;
    pdata->prev=dummypdata;
    pdata->thresholdN=25;
	pdata->thresholdD=10000; // 0.0025
    pdata->nodes=0;
    pdata->calls=0;
    pdata->growth=0;
    #if PRUNE_BY_CALLS
    dummypdata->nodes=0;
    dummypdata->calls=4096; // pruning starts at 4096*2^i calls
    #elif PRUNE_BY_NODES
    dummypdata->nodes=1000; // pruning starts at 1000*1.5^i nodes
    dummypdata->calls=0;
    #endif
    dummypdata->growth=0;

	#if !USE_MALLOC
    pdata->freeList=(CCTNode*)malloc(sizeof(CCTNode));
	pdata->freeList->firstChild=NULL;
	dummypdata->freeList=pdata->freeList; // HERE
    #endif
	// thread data
    tdata->numNodes      = 0;
    tdata->numCalls      = 0;
    tdata->threadID      = threadid;
	tdata->pdata         = pdata;
    tdata->callStackTop  = 0;
    tdata->callStackSize = INIT_STACK;
    tdata->callStack     = (ActivationRec*)malloc(tdata->callStackSize*sizeof(ActivationRec));
    if (tdata->callStack == NULL) exit((printf("[TID=%u] Can't allocate stack\n", threadid),1));
    
    // allocate sentinel
    CCTNode* theNodePtr = (CCTNode*)calloc(sizeof(CCTNode), 1);
    if (theNodePtr == NULL) exit((printf("[TID=%u] Can't allocate stack node\n", threadid),1));
	
    // dummy parent => no (parent!=NULL) anymore :)
    //~ #if USE_MALLOC
    CCTNode* virtualParent=(CCTNode*)malloc(sizeof(CCTNode));
	//~ #else
	//~ CCTNode* virtualParent=a_malloc(pdata);
    //~ #endif
    virtualParent->parent=NULL;
	virtualParent->firstChild=theNodePtr;
	virtualParent->nextSibling=NULL;
	virtualParent->count=0;
    #if USE_COUNTCHID
	virtualParent->countChild=0;
	virtualParent->update=0;
    #endif
    virtualParent->error=0;
	virtualParent->errorChild=0;
    
	// init node
	theNodePtr->parent=virtualParent;
	theNodePtr->firstChild=NULL;
	theNodePtr->nextSibling=NULL;
	theNodePtr->count=0;
    #if USE_COUNTCHILD
	theNodePtr->countChild=0;
	theNodePtr->update=0;
    #endif
    theNodePtr->error=0;
	theNodePtr->errorChild=0;

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
	#if CORRECT_HCCT
		#if CALLING_SITE
		sprintf(fileName, "hcct-%s-%u.cs.out", appName, threadid);
		#else
		sprintf(fileName, "hcct-%s-%u.out", appName, threadid);
		#endif
	#else
		#if CALLING_SITE
		sprintf(fileName, "hcct-%s-%u.cs.out.batch", appName, threadid);
		#else
		sprintf(fileName, "hcct-%s-%u.out.batch", appName, threadid);
		#endif
	#endif

    // print message
    printf("[%s] creating output file...\n", fileName);

    // create output file
    out = fopen(fileName, "w");
    if (out == NULL) exit((printf("can't create output file %s\n", fileName), 1));

    // write output file
    WriteOutput(tdata, out);

    // close output file
    fclose(out);

	// dump tree
	#if DUMP_TREE

    FILE*  dump;
    char dumpFileName[32]; 

	#if CORRECT_HCCT
		#if CALLING_SITE    
		sprintf(dumpFileName, "hcct-%s-%u.cs.dump", appName, threadid);
		#else
		sprintf(dumpFileName, "hcct-%s-%u.dump", appName, threadid);
		#endif
	#else
		#if CALLING_SITE    
		sprintf(dumpFileName, "hcct-%s-%u.cs.dump.batch", appName, threadid);
		#else
		sprintf(dumpFileName, "hcct-%s-%u.dump.batch", appName, threadid);
		#endif
	#endif

    dump = fopen(dumpFileName, "w");
    if (dump == NULL) exit((printf("can't create output file %s\n", dumpFileName), 1));

	dumpTree(tdata->callStack[0].node, dump, tdata);

	fclose(dump);
    #endif

    // free CCT
    freeTree(tdata->callStack[0].node);

    // free call stack
    free(tdata->callStack);

    // free thread local data
    free(tdata);
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
    #if USE_MALLOC
	theNodePtr = (CCTNode*)calloc(sizeof(CCTNode), 1);
	#else
	theNodePtr=a_malloc(tdata->pdata);
    #endif
    if (theNodePtr == NULL) exit((printf("[TID=%u] Out of memory error\n", threadid),1));
    
	// node info
	theNodePtr->parent= tdata->callStack[tdata->callStackTop].node;
	// FIX BELOW???
	//theNodePtr->count=theNodePtr->parent->errorChild; // will be increased later
	theNodePtr->count=theNodePtr->parent->errorChild; // will be increased later
    theNodePtr->error=theNodePtr->count; // sovrastima "precisa"
	theNodePtr->errorChild=theNodePtr->count; // FIX???
	// HERE
	theNodePtr->firstChild=NULL;
    #if USE_COUNTCHILD
	theNodePtr->countChild=0;
	theNodePtr->update=0; // will be increased later
    #endif
        
    // add node to tree
    tdata->pdata->nodes++;
    tdata->pdata->growth++;
    tdata->numNodes++;
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
    
    // update prune data
    tdata->numCalls++;
    tdata->pdata->calls++;
            
	#if USE_COUNTCHILD
	// update
	theNodePtr->update++;
	#endif
    
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

    // Pruning time?
    #if PRUNE_BY_CALLS
    if (tdata->pdata->calls >= 2*tdata->pdata->prev->calls) {
    #elif PRUNE_BY_NODES
    if (tdata->pdata->nodes >= 2*tdata->pdata->prev->nodes) {
    #endif
        PruneData* newpdata=(PruneData*)malloc(sizeof(PruneData));
        newpdata->prev=tdata->pdata;
		#if !USE_MALLOC
		newpdata->freeList=tdata->pdata->freeList;
		#endif
        tdata->pdata=newpdata;
        newpdata->thresholdN=tdata->pdata->prev->thresholdN;
		newpdata->thresholdD=tdata->pdata->prev->thresholdD;
        newpdata->nodes=tdata->pdata->prev->nodes; // current #nodes, not "global"
        newpdata->calls=tdata->numCalls; // "global" #calls
        newpdata->nodeSP=tdata->callStack[tdata->callStackTop].node;
        newpdata->growth=0;
        Prune(tdata->callStack[0].node, newpdata); 
    }

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
