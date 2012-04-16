#ifndef __ANALYSIS__
#define __ANALYSIS__

#include "common.h"

#define BUF 500

// Discriminate tool used to create logfile
#define CCT_FULL    1
#define LSS_FULL    2
#define CCT_BURST   3
#define LSS_BURST   4

#define CCT_FULL_STRING     "cct"
#define LSS_FULL_STRING     "lss-hcct"
#define CCT_BURST_STRING    "cct-burst"
#define LSS_BURST_STRING    "lss-hcct-burst"

typedef struct hcct_node_s hcct_node_t;
struct hcct_node_s {
    UINT32          routine_id;
    UINT16          call_site;
    UINT32          counter;
    //char*         info_routine;    // DA IMPLEMENTARE!
    
    hcct_node_t*    first_child;
    hcct_node_t*    next_sibling;
    hcct_node_t*    parent; // useful?
};

typedef struct hcct_pair_s hcct_pair_t;
struct hcct_pair_s {
    hcct_node_t*    node;
    UINT32          id;
};

typedef struct hcct_tree_s hcct_tree_t;
struct hcct_tree_s {
    hcct_node_t*    root;
    unsigned short  tool;
    UINT32          nodes;
    UINT32          sampling_interval;
    UINT32          burst_length;
    UINT32          epsilon;
    UINT32          phi;
    UINT64			enter_events;
    UINT64			burst_enter_events; // 0 if exhaustive analysis    
};

#endif
