#ifndef __ANALYSIS__
#define __ANALYSIS__

#include <sys/types.h> // pid_t

#include "common.h"

// Discriminate tool used to create logfile
#define CCT_FULL    1
#define LSS_FULL    2
#define CCT_BURST   3
#define LSS_BURST   4
#define CCT_TIME	5
#define LSS_TIME	6

#define CCT_FULL_STRING     "cct"
#define LSS_FULL_STRING     "lss-hcct"
#define CCT_BURST_STRING    "cct-burst"
#define LSS_BURST_STRING    "lss-hcct-burst"
#define CCT_TIME_STRING		"cct-time"
#define LSS_TIME_STRING		"lss-hcct-time"

typedef struct hcct_sym_s hcct_sym_t;
struct hcct_sym_s {
	char*	name;	// addr2line or <unknown> or address
	char*	file;	// addr2line or range
	char*	image;	// from /proc/self/maps or program_short_name
};

typedef struct hcct_node_s hcct_node_t;
struct hcct_node_s {
    ADDRINT         routine_id;
    ADDRINT         call_site;
    UINT32          counter;
    hcct_sym_t*		routine_sym;
    hcct_sym_t*		call_site_sym;        
    hcct_node_t*    first_child;
    hcct_node_t*    next_sibling;
    hcct_node_t*    parent;
};

typedef struct hcct_pair_s hcct_pair_t;
struct hcct_pair_s {
    hcct_node_t*    node;
    ADDRINT          id;
};

typedef struct hcct_map_s hcct_map_t;
struct hcct_map_s {
	ADDRINT		start;
	ADDRINT		end;	
	char*		pathname;
	ADDRINT		offset; // offset into the file - TODO: useles??
	hcct_map_t*	next;
};


typedef struct hcct_tree_s hcct_tree_t;
struct hcct_tree_s {
    hcct_node_t*    root;
    char			*short_name;
    char			*program_path;
    hcct_map_t*		map;
    pid_t			tid;
    unsigned short  tool;    
    UINT32          nodes;
    UINT32          sampling_interval;
    UINT32          burst_length;
    double          epsilon;
    double          phi;
    UINT64			sampled_enter_events;
    UINT64			exhaustive_enter_events;
    UINT64			sum_of_tics;
    UINT64			thread_tics;
};

#endif
