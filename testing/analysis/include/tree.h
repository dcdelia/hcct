#ifndef __TREE__
#define __TREE__

typedef struct hcct_node_s hcct_node_t;
typedef struct hcct_pair_s hcct_pair_t;
typedef struct hcct_tree_s hcct_tree_t;

#include <stdio.h>
#include "common.h"

#define NODE_BUFFER_SIZE            4096    // buffer for sorting - benchmark Botan has max degree 1022!
#define SOLVE_PROGRESS_INDICATOR    100

hcct_tree_t* loadTree(char* filename);
void freeTree(hcct_tree_t* tree);
void pruneTreeAboveCounter(hcct_tree_t* tree, UINT32 min_counter);
void pruneTreeAtDepth(hcct_tree_t* tree, UINT32 depth);
void solveTreeFromExistingMap(hcct_tree_t* tree, syms_map_t* syms_map);
void solveTree(hcct_tree_t* tree, char* program, char* maps_file, char* syms_file, syms_map_t** syms_map_ptr);
syms_map_t* solveTreeFromMapsFile(hcct_tree_t* tree, char* maps_file, char* program);
void sortChildrenByDecreasingFrequency(hcct_tree_t* tree);
double getAvgDegree(hcct_tree_t* tree);
UINT32 getMaxDegree(hcct_tree_t* tree);
double getPhi(hcct_tree_t* tree, double candidatePhi);
UINT32 getMinThreshold(hcct_tree_t* tree, double phi);
UINT64 sumCountersInSubtree(hcct_node_t* node);
double getAdjustFactor(hcct_tree_t* first_tree, hcct_tree_t* second_tree);

struct hcct_node_s {
    ADDRINT routine_id;
    ADDRINT call_site;
    ADDRINT node_id;
    UINT32 counter;
    sym_t* routine_sym;
    sym_t* call_site_sym;
    hcct_node_t* first_child;
    hcct_node_t* next_sibling;
    hcct_node_t* parent;
    void* extra_info;
};

struct hcct_pair_s {
    hcct_node_t* node;
    ADDRINT id;
};

struct hcct_tree_s {
    hcct_node_t* root;
    char *short_name;
    char *program_path;
    pid_t tid;
    unsigned short tool;
    UINT32 nodes;
    UINT32 sampling_interval;
    UINT32 burst_length;
    double epsilon;
    double phi;
    UINT64 sampled_enter_events;
    UINT64 exhaustive_enter_events;
    UINT64 sum_of_tics;
    UINT64 thread_tics;
};

#endif

