#ifndef STREAMING_H
#define STREAMING_H

#include <stdlib.h>
#include <stdio.h>
#include "STool.h"

#define CALL_SITE   0   // if 1, treat calls from different calling sites as different
#define STREAMING	1	// enable streaming algorithm (HCCT)
#define ST_DUMP     1   // dump streaming algorithm's data
#define DUMP_TREE   1   // if 1, dump to file the [H]CCT
#define DUMP_STAT   1   // if 1, compute and write statistics about the [H]CCT to file
#define PRUNING		1	// if 0, mantains the whole CCT with streaming info

extern short fatNodeBytes;
extern short fatThreadBytes;

// Record associated with CCT node
typedef struct CCTNode {
    struct CCTNode* parent;         // parent of the node in the HCCT
    struct CCTNode* firstChild;     // first child of the node in the HCCT
    struct CCTNode* nextSibling;    // next sibling of the node in the HCCT
    unsigned int    count;          // number of times the context is reached 
    ADDRINT         target;         // address of call target within function associated with CCT node
    #if CALL_SITE
    ADDRINT         callSite;       // address of calling instruction
    #endif
} CCTNode;

// Thread local data record
typedef struct ThreadRec {
    CCTNode* root;      // root of the CCT
	#if STREAMING==0
	int numNodes;		// number of nodes in use
	long numCalls;		// number of calls
	#endif
} ThreadRec;

// Public functions
void	ST_new(ThreadRec* tdata);
void	ST_update(ThreadRec* tdata, CCTNode* theNodePtr);
void	ST_free(ThreadRec* tdata);
void	ST_dumpStats(ThreadRec* tdata, FILE* out);
void	ST_dumpNode(CCTNode* root, FILE* out);
#if STREAMING
int		ST_nodes(ThreadRec* tdata);
long	ST_length(ThreadRec* tdata);
#endif

#endif
