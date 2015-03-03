#ifndef __COMPARE
#define	__COMPARE

typedef struct compare_info_s compare_info_t;
typedef struct compare_heuristic_info_s compare_heuristic_info_t;

#include "common.h"
#include <glib.h>

// internal parameters
#define HEURISTIC_THRESHOLD_FULL    9       // having a node with degree greater than the threshold might trigger apx matching
#define HEURISTIC_THRESHOLD_QUICK   12
#define MATCH_PROGRESS_INDICATOR    100     // print progress information every PROGRESS_INDICATOR matched nodes

// statuses for nodes
#define STATUS_NOT_EVALUATED    0       // not evaluated
#define STATUS_MATCHED          1       // (first tree) matched
#define STATUS_APX_MATCHED      2       // (first tree) matched using apx algorithm
#define STATUS_NOT_MATCHED      3       // (first tree) no match found

// similarity between counters
//#define COUNTER_NOISE_THRESHOLD        0.01    // two counters are treates as identical if they differ at most by this ratio (wrt first)
//#define COUNTER_UPPER_THRESHOLD        0.1     // counters should differ by no more than this ratio (wrt first)
// TODO: bursting, re-implement noise and upper?

// weights for local score (they must add up to 100) [originally they were 15 5 5 15 - 15 5 5 15 - 20]
#define WEIGHT_ROUTINE_NAME     10
#define WEIGHT_ROUTINE_FILE     10
#define WEIGHT_ROUTINE_IMAGE    10
#define WEIGHT_ROUTINE_OFFSET   10
#define WEIGHT_CALLSITE_NAME    10
#define WEIGHT_CALLSITE_FILE    10
#define WEIGHT_CALLSITE_IMAGE   10
#define WEIGHT_CALLSITE_OFFSET  10
#define WEIGHT_COUNTER          20

// weight for non-local score (apx matching only)
#define WEIGHT_SUBTREE_SUM      0.15

struct compare_info_s {
    int status;
    double score;
    hcct_node_t* matched_node;
    UINT64 subtree_weight;
};

struct compare_heuristic_info_s {
    GHashTable* first_subtree_weights_map;
    UINT64* first_subtree_weights;
    UINT32  first_nodes; // before any pruning

    GHashTable* second_subtree_weights_map;
    UINT64* second_subtree_weights;
    UINT32  second_nodes; // before any pruning
};

#endif