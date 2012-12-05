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
    char*			info;
    
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
    char			*short_name;
    char			*program_path;
    UINT32			tid;
    unsigned short  tool;    
    UINT32          nodes;
    UINT32          sampling_interval;
    UINT32          burst_length;
    UINT32          epsilon;
    UINT32          phi;
    UINT64			enter_events; // total number of [sampled] rtn enter events
    UINT64			burst_enter_events; /* total number of rtn enter events
									(0 if exhaustive analysis is performed) */
};

typedef struct hcct_map_s hcct_map_t;
struct hcct_map_s {
	UINT32		start;
	UINT32		end;	
	char*		pathname;
	UINT32		offset; // offset into the file - man proc (5)
	hcct_map_t*	next;
};

#endif
