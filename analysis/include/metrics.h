#ifndef __METRICS__
#define	__METRICS__

#include "common.h"

UINT32 countNodes(hcct_node_t* root);
UINT32 hotNodes(hcct_node_t* root, UINT32 threshold);
UINT64 sumCounters(hcct_node_t* root);
UINT32 hottestCounter(hcct_node_t* root);
UINT32 largerThanHottest(hcct_node_t* root, UINT32 TAU, UINT32 hottest);

#endif

