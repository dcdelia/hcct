#include <math.h>
#include <glib.h>
#include <string.h>

#include "common.h"
#include "compare.h"
#include "compare_metrics.h"
#include "metrics.h"
#include "tree.h"

/*
 * Prototypes for helping routines.
 */

static void computeHotEdgeCoverageAux(hcct_node_t* node, UINT32* thresholds, UINT32* nodes, UINT32* matched, size_t len);
static void computeNodePartitioningAuxHotColdFalsePositives(hcct_node_t* hcct_node, GHashTable* hash, UINT32 min_cct, UINT32 min_hcct,
                                        UINT32* hot_nodes,UINT32* cold_nodes, UINT32* false_positives);
static void computeNodePartitioningAuxFalseNegatives(hcct_node_t* cct_node, UINT32 cct_threshold, UINT32* false_negatives);
static void computeUncoveredFrequencyAux(hcct_node_t* node, UINT32* max, UINT32* uncovered, UINT64* sum);
static void computeCounterAccuracyAux(hcct_node_t* node, UINT32 hot_threshold, UINT32 error_threshold, double factor,
                                        UINT32* hot_nodes, double* max, double* sum, UINT32* fixed_under, UINT32* fixed_over, UINT32* fixed_over_100);
static void constructHashTable(hcct_node_t* node, GHashTable* hash);
static UINT32 checkNodesForFlag(hcct_node_t* node, int flag);
static void checkNodesForThreshold(hcct_node_t* node, double* thresholds, UINT32* results, size_t len);

/*
 * Routines that perform measurements.
 */

// computes counter accuracy for hot nodes only
void computeCounterAccuracy(hcct_tree_t* cct, hcct_tree_t* hcct, double phi, double* max, double* avg, double* factor,
                            UINT32* matched, UINT32* fixed_under, UINT32* fixed_over, UINT32* fixed_over_100) {
    UINT32 t_nodes = 0, t_fixed_under = 0, t_fixed_over = 0, t_fixed_over_100 = 0;
    double t_sum = 0;
    double t_max = 0;

    UINT32 hot_threshold = getMinThreshold(cct, getPhi(cct, phi));
    double t_factor = getAdjustFactor(cct, hcct);

    UINT32 error_threshold;

    if (hcct->tool == CCT_FULL || hcct->tool == LSS_FULL) {
        if (hcct->tool == CCT_FULL) {
            // when I compare two CCTs I don't have any theoretical bound on error
            error_threshold = (UINT32)-1;
        } else {
            error_threshold = hcct->epsilon * cct->exhaustive_enter_events;
        }
    } else if (hcct->tool == CCT_BURST || hcct->tool == LSS_BURST) {
        // in both cases I don't have any theoretical bound on error
        error_threshold = (UINT32)-1;
    } else {
        // CCT_TIME and LSS_TIME are not actively developed anymore
        *max = *avg = *factor = *matched = *fixed_under = *fixed_over = *fixed_over_100 = 0;
        printf("Sorry, this has not been implemented yet!\n");
        return;
    }

    computeCounterAccuracyAux(cct->root, hot_threshold, error_threshold, t_factor,
                            &t_nodes, &t_max, &t_sum, &t_fixed_under, &t_fixed_over, &t_fixed_over_100);

    *max = t_max;
    *avg = t_sum;
    if (t_nodes > 1) {
        *avg = (*avg)/(double)t_nodes;
    }
    *factor = t_factor;
    *fixed_under = t_fixed_under;
    *fixed_over = t_fixed_over;
    *fixed_over_100 = t_fixed_over_100;
    *matched = t_nodes;
}

void computeHotEdgeCoverage(hcct_tree_t* cct, UINT32 max, double* taus, UINT32* nodes, UINT32 *matched, size_t len) {
    UINT32* thresholds = malloc(len*sizeof(UINT32));
    int i;
    for (i = 0; i < len; ++i) {
        thresholds[i] = ceil(taus[i]*max);
    }

    memset(nodes, 0, len*sizeof(UINT32));
    memset(matched, 0, len*sizeof(UINT32));

    computeHotEdgeCoverageAux(cct->root, thresholds, nodes, matched, len);

    free(thresholds);
}

void computeNodePartitioning(hcct_tree_t* cct, hcct_tree_t* hcct, double phi, UINT32* hot_nodes,
                                UINT32* cold_nodes, UINT32* false_positives, UINT32* false_negatives) {
    UINT32 min_cct = getMinThreshold(cct, getPhi(cct, phi));
    UINT32 min_hcct = getMinThreshold(hcct, getPhi(hcct, phi));

    if (false_negatives != NULL) { // optional field
        *false_negatives = 0;
        computeNodePartitioningAuxFalseNegatives(cct->root, min_cct, false_negatives);
    }

    // construct a GHashTable for fast reverse lookup
    GHashTable* hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    constructHashTable(cct->root, hash);

    *hot_nodes = *cold_nodes = *false_positives = 0;

    computeNodePartitioningAuxHotColdFalsePositives(hcct->root, hash, min_cct, min_hcct, hot_nodes, cold_nodes, false_positives);

    g_hash_table_destroy(hash);
}

void computeUncoveredFrequency(hcct_tree_t* cct, UINT32* max, double* avg) {
    UINT32 t_max = 0, t_uncovered = 0;
    UINT64 t_sum = 0;

    computeUncoveredFrequencyAux(cct->root, &t_max, &t_uncovered, &t_sum);

    *avg = t_sum;
    if (t_uncovered > 0) {
        *avg /= (double)t_uncovered;
    }
    *max = t_max;
}

/*
 * Helping routines to perform measurements.
 */

static void computeHotEdgeCoverageAux(hcct_node_t* node, UINT32* thresholds, UINT32* nodes, UINT32* matched, size_t len) {
    int index;
    for (index = 0; index < len; ++index) {
        if (node->counter >= thresholds[index]) {
            ++nodes[index];
            if (((compare_info_t*) node->extra_info)->matched_node != NULL) {
                ++matched[index];
            }
        }
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        computeHotEdgeCoverageAux(child, thresholds, nodes, matched, len);
}

static void constructHashTable(hcct_node_t* node, GHashTable* hash) {
    compare_info_t* info = (compare_info_t*)node->extra_info;
    if (info->matched_node != NULL) {
        g_hash_table_insert(hash, info->matched_node, node);
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        constructHashTable(child, hash);
}

static void computeNodePartitioningAuxHotColdFalsePositives(hcct_node_t* hcct_node, GHashTable* hash, UINT32 min_cct, UINT32 min_hcct,
                                        UINT32* hot_nodes,UINT32* cold_nodes, UINT32* false_positives) {

    hcct_node_t* cct_node = (hcct_node_t*)g_hash_table_lookup(hash, hcct_node);
    if (cct_node == NULL) {
        if (hcct_node->counter > min_hcct) { // false positive
            ++(*false_positives);
        } else { // ancestor of a false positive
            ++(*cold_nodes);
        }
    } else {
        if (cct_node->counter > min_cct) {  // should work for bursting
            ++(*hot_nodes);
        } else {
            ++(*cold_nodes);
        }
    }

    hcct_node_t* child;
    for (child = hcct_node->first_child; child != NULL; child = child->next_sibling)
        computeNodePartitioningAuxHotColdFalsePositives(child, hash, min_cct, min_hcct, hot_nodes, cold_nodes, false_positives);
}

static void computeNodePartitioningAuxFalseNegatives(hcct_node_t* cct_node, UINT32 cct_threshold, UINT32* false_negatives) {
    if (cct_node->counter > cct_threshold) {
        hcct_node_t* other = ((compare_info_t*)cct_node->extra_info)->matched_node;
        if (other == NULL) ++(*false_negatives);
    }
    hcct_node_t* child;
    for (child = cct_node->first_child; child != NULL; child = child->next_sibling) {
        computeNodePartitioningAuxFalseNegatives(child, cct_threshold, false_negatives);
    }
}

static void computeUncoveredFrequencyAux(hcct_node_t* node, UINT32* max, UINT32* uncovered, UINT64* sum) {
    if (((compare_info_t*) node->extra_info)->matched_node == NULL) {
        ++(*uncovered);
        (*sum) += node->counter;
        if (node->counter > (*max)) {
            (*max) = node->counter;
        }
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        computeUncoveredFrequencyAux(child, max, uncovered, sum);
}

static void computeCounterAccuracyAux(hcct_node_t* node, UINT32 hot_threshold, UINT32 error_threshold, double factor,
                                        UINT32* hot_nodes, double* max, double* sum, UINT32* under, UINT32* over, UINT32* over_100) {
    if (node->counter > hot_threshold) {
        hcct_node_t* other = ((compare_info_t*) node->extra_info)->matched_node;
        if (other != NULL) {
            ++(*hot_nodes);
            UINT32 adjusted_counter = (UINT32)(other->counter * factor);
            UINT32 diff = ABSDIFF(adjusted_counter, node->counter);
            if (diff > error_threshold) { // fix it: it would screw avg and max error!
                if (adjusted_counter > node->counter) ++(*over);
                else ++(*under);
                diff = error_threshold;
            }
            double error = diff/(double)node->counter;
            if (error > 1.0) {
                //printf("[WARNING] difference in counters greater than 100 for current node%\n");
                //printf("--> %lu@%s vs %lu@%s\n", node->counter, node->routine_sym->name, adjusted_counter, other->routine_sym->name);
                ++(*over_100);
                error = 1.0;
            }
            if (error > *max) *max = error;
            if (error > 0) *sum += error;
        }
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        computeCounterAccuracyAux(child, hot_threshold, error_threshold, factor, hot_nodes, max, sum, under, over, over_100);
}

/*
 * Routines that print statistics to screen.
 */

void printMatchingStats(hcct_tree_t* first_tree, hcct_tree_t* second_tree, double* thresholds, size_t len) {
    UINT32* results = calloc(len, sizeof(UINT32));
    UINT32 matched;

    printf("-------------------------\n");
    printf("Summary of the comparison\n");
    printf("-------------------------\n");
    printf("Unmatched nodes from first tree: %lu\n", checkNodesForFlag(first_tree->root, STATUS_NOT_MATCHED));
    matched = checkNodesForFlag(first_tree->root, STATUS_MATCHED) + checkNodesForFlag(first_tree->root, STATUS_APX_MATCHED);
    printf("Unmatched nodes from second tree: %lu\n", second_tree->nodes - matched);
    printf("Matched nodes: %lu\n", matched);
    checkNodesForThreshold(first_tree->root, thresholds, results, len);
    int index;
    for (index = 0; index < len; ++index) {
        printf("%.0f%%-matched nodes: %.2f%% (%lu)\n", 100*thresholds[index], 100*results[index]/(double)matched, results[index]);
    }
    printf("\n");
    free(results);
}


void printFalseColdHotStats(hcct_tree_t* tree_1, hcct_tree_t* tree_2, double phi, int prune) {
    printf("--------------------------\n");
    printf("Node partitioning analysis\n");
    printf("--------------------------\n");

    UINT32 min_threshold_1 = getMinThreshold(tree_1, getPhi(tree_1, phi));

    if (prune) { // I don't want to prune twice :-)
        pruneTreeAboveCounter(tree_1, min_threshold_1);
    }

    UINT32 hot_nodes, cold_nodes, false_positives, false_negatives;
    computeNodePartitioning(tree_1, tree_2, phi, &hot_nodes, &cold_nodes, &false_positives, &false_negatives);

    UINT32 first_hot_nodes = hotNodes(tree_1->root, min_threshold_1);

    printf("<exact HCCT>\n");
    printf("Number of nodes: %lu\n", tree_1->nodes);
    printf("Number of cold and hot nodes: %lu %lu\n", tree_1->nodes - first_hot_nodes, first_hot_nodes);
    printf("<second tree>\n");
    printf("Number of nodes: %lu\n", hot_nodes+cold_nodes+false_positives);
    printf("Number of false negatives: %lu\n", false_negatives);
    printf("Number of cold, hot, and false positives: %lu %lu %lu\n", cold_nodes, hot_nodes, false_positives);
    printf("Percentage of cold, hot, and false positives: %.2f %.2f %.2f\n",
            (cold_nodes/(double)tree_2->nodes)*100, (hot_nodes/(double)tree_2->nodes)*100, (false_positives/(double)tree_2->nodes)*100);
    printf("\n");
}

void printAccuracyStats(hcct_tree_t* cct, hcct_tree_t* hcct, double phi) {
    printf("-------------------------\n");
    printf("Counter accuracy analysis\n");
    printf("-------------------------\n");

    double max_error, avg_error, factor;
    UINT32 under, over, over_100, matched;
    computeCounterAccuracy(cct, hcct, phi, &max_error, &avg_error, &factor, &matched, &under, &over, &over_100);

    if (hcct->tool == CCT_BURST || hcct->tool == LSS_BURST) {
        printf("Fraction of sampled events: %f\n", hcct->sampled_enter_events/(double)hcct->exhaustive_enter_events);
        printf("Ratio between sampled events and tree 1's events: %f\n", hcct->sampled_enter_events/(double)cct->exhaustive_enter_events);
        printf("Ratio between total events and tree 1's events: %f\n", hcct->exhaustive_enter_events/(double)cct->exhaustive_enter_events);
        printf("Matched hot nodes: %lu\n", matched);
    }

    printf("Avg and max %% counter error on hot nodes: %.6f %.6f\n", avg_error*100, max_error*100);
    printf("Factor used to adjust counters: %f (%f)\n", factor, 1.0/factor);
    printf("Nodes underestimated beyond error threshold: %lu\n", under);
    printf("Nodes overestimated beyond error threshold: %lu\n", over);
    printf("Nodes overestimated beyond 100%% of first tree's counter: %lu\n", over_100);
    printf("\n");
}

// to be invoked before pruning the second tree
void printUncoveredStats(hcct_tree_t* cct) {
    printf("----------------------------\n");
    printf("Uncovered frequency analysis\n");
    printf("----------------------------\n");

    UINT32 cct_max = hottestCounter(cct->root); // for normalization
    UINT32 max_uncovered;
    double avg_uncovered;
    computeUncoveredFrequency(cct, &max_uncovered, &avg_uncovered);

    printf("Hottest counter frequency: %lu\n", cct_max);
    printf("Avg and max uncovered counter frequency: %.2f %lu\n", avg_uncovered, max_uncovered);
    printf("Avg and max %% uncovered c.f. wrt hottest: %.6f %.6f\n", avg_uncovered/(double)cct_max*100, max_uncovered/(double)cct_max*100);
    printf("\n");
}

// to be invoked before pruning the second tree
void printHotEdgeCoverageStats(hcct_tree_t* cct, double phi) {
    printf("--------------------------\n");
    printf("Hot edge coverage analysis\n");
    printf("--------------------------\n");

    int i;
    const double factors[] = {0.01, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4, 1.5};
    size_t len = sizeof(factors)/sizeof(double);

    double* taus = malloc(len*sizeof(double));

    UINT32 cct_min_threshold = getMinThreshold(cct, getPhi(cct, phi));
    UINT32 cct_max = hottestCounter(cct->root);

    double min_tau = (cct_min_threshold+1)/(double)cct_max;

    printf("Min tau for which I can expect 100%% coverage: %.8f\n", min_tau);

    // create array of tau thresholds
    for (i = 0; i < len; ++i) {
        taus[i] = min_tau / factors[i];
    }

    UINT32* nodes = malloc(len * sizeof(UINT32));
    UINT32* matched = malloc(len * sizeof(UINT32));

    computeHotEdgeCoverage(cct, cct_max, taus, nodes, matched, len);

    for (i = 0; i < len; ++i) {
        double score = 100;
        if (nodes[i] > 0) {
            score *= matched[i]/(double)nodes[i];
        }
        printf("%.2f times min_tau: %.2f%% (%lu/%lu)\n", 1.0/factors[i], score, matched[i], nodes[i]);
    }
    printf("\n");

    free(nodes);
    free(matched);
    free(taus);
}

/*
 * Hepling routines to print statistics.
 */

// determine how many nodes have some flag set
static UINT32 checkNodesForFlag(hcct_node_t* node, int flag) {
    UINT32 ret = 0;

    if (((compare_info_t*) node->extra_info)->status == flag) {
        ++ret;
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        ret += checkNodesForFlag(child, flag);
    }

    return ret;
}

// determine how many nodes have score above or equal to the given threshold
static void checkNodesForThreshold(hcct_node_t* node, double* thresholds, UINT32* results, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        if (((compare_info_t*) node->extra_info)->score >= thresholds[i]) ++results[i];
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        checkNodesForThreshold(child, thresholds, results, len);
    }
}
