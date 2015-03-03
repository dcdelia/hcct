#ifndef __COMPARE_METRICS
#define	__COMPARE_METRICS

#include <stdlib.h> // size_t
#include <glib.h>

#include "common.h"

void computeHotEdgeCoverage(hcct_tree_t* cct, UINT32 max, double* taus, UINT32* nodes, UINT32 *matched, size_t len);
void computeNodePartitioning(hcct_tree_t* cct, hcct_tree_t* hcct, double phi, UINT32* hot_nodes,
                                UINT32* cold_nodes, UINT32* false_positives, UINT32* false_negatives);
void computeUncoveredFrequency(hcct_tree_t* cct, UINT32* max, double* avg);
void computeCounterAccuracy(hcct_tree_t* cct, hcct_tree_t* hcct, double phi, double* max, double* avg, double* factor,
                            UINT32* matched, UINT32* fixed_under, UINT32* fixed_over, UINT32* fixed_over_100);
double computeCounterAccuracyRaw(hcct_tree_t* cct, hcct_tree_t* hcct, double phi);

void printMatchingStats(hcct_tree_t* first_tree, hcct_tree_t* second_tree, double* thresholds, size_t len);
void printFalseColdHotStats(hcct_tree_t* tree_1, hcct_tree_t* tree_2, double phi, int prune);
void printHotEdgeCoverageStats(hcct_tree_t* cct, double phi);
void printAccuracyStats(hcct_tree_t* cct, hcct_tree_t* hcct, double phi);
void printUncoveredStats(hcct_tree_t* cct);


#endif

