#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compare.h"
#include "compare_metrics.h"
#include "comb.h"
#include "metrics.h"
#include "output.h"
#include "symbols.h"
#include "tree.h"

// compute similarity between two symbols
static inline int compareSym(sym_t* first, sym_t* second, int w_name, int w_file, int w_image, int w_offset) {
    int score = 0;
    if (strcmp(first->name, SYM_UNKNOWN) && strcmp(second->name, SYM_UNKNOWN)) {
        // I can compare images and offsets in any case!
        int images = strcmp(first->image, second->image);
        if (!images && strcmp(first->image, SYM_UNRESOLVED_IMAGE)) {
            score += w_image;
            if (first->offset == second->offset) {
                score += w_offset;
            }
            int names = strcmp(first->name, second->name);
            if (!names && strcmp(first->name, SYM_UNRESOLVED_NAME)) {
                score += w_name;
                if (!strcmp(first->file, second->file)) {
                    score += w_file;
                }
            }
        }
    }
    return score;
}

// compute similarity between two nodes (using local properties only)
static double compareNodesLocally(hcct_node_t* first, hcct_node_t* second, double adjust_factor) {
    double score = 0.0;
    if (first != NULL && second != NULL) {
        score += compareSym(first->routine_sym, second->routine_sym, WEIGHT_ROUTINE_NAME, WEIGHT_ROUTINE_FILE, WEIGHT_ROUTINE_IMAGE, WEIGHT_ROUTINE_OFFSET)/100.0;
        score += compareSym(first->call_site_sym, second->call_site_sym, WEIGHT_CALLSITE_NAME, WEIGHT_CALLSITE_FILE, WEIGHT_CALLSITE_IMAGE, WEIGHT_CALLSITE_OFFSET)/100.0;

        /* Compare counters */
        double c_score = 0.0;

        // coarse-grained version
        //if (first->counter == second->counter) c_score = WEIGHT_COUNTER;

        // be careful: integers are unsigned!
        UINT32 diff = ABSDIFF(first->counter, adjust_factor * second->counter);
        double ratio = diff / (double)first->counter;
        if (ratio < 1.0) {
            c_score += (1.0 - ratio) * WEIGHT_COUNTER;
        }

        score += c_score/100.0;
    }
    return score;
}

static void setSubtreeAsNotMatched(hcct_node_t* node) {
    compare_info_t* info = node->extra_info;
    if (info->status != STATUS_NOT_EVALUATED || info->matched_node != NULL) {
        printf("\n[ERROR] a branch of the tree had already been analyzed!\n");
        int depth = 0;
        hcct_node_t* tmp = node;
        while (tmp != NULL) {
            tmp = tmp->parent;
            depth++;
        }
        printf("Depth of the node: %d\n", depth);
        exit(1);
    }

    info->status = STATUS_NOT_MATCHED;

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        setSubtreeAsNotMatched(child);
    }
}

static void resetCompareInfo(hcct_node_t* node) {
    compare_info_t* info = (compare_info_t*) node->extra_info;
    info->status = STATUS_NOT_EVALUATED;
    info->score = 0;
    info->matched_node = NULL;

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        resetCompareInfo(child);
}

// ignore return value (used for internal recursion only)
static UINT64 collectSubtreeInfoAux(hcct_node_t* node, GHashTable* map, UINT64* array, UINT32* index) {
    UINT64 sum = 0;
    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        sum += collectSubtreeInfoAux(child, map, array, index);
    }

    if (node->extra_info == NULL) { // we are in the second tree: use hash map
        g_hash_table_insert(map, node, array + *index);
        array[(*index)++] = sum;
    } else { // we are in the first tree: use ad-hoc allocated field
        compare_info_t* info = (compare_info_t*)node->extra_info;
        info->subtree_weight = sum;
        ++(*index);
    }

    return sum + node->counter;
}

static compare_heuristic_info_t* collectHeuristicInfo(hcct_tree_t* first_tree, hcct_tree_t* second_tree) {
    UINT32 index;

    compare_heuristic_info_t* heuristic_info = calloc(1, sizeof(compare_heuristic_info_t));
    heuristic_info->first_nodes = first_tree->nodes;

    index = 0;
    collectSubtreeInfoAux(first_tree->root, heuristic_info->first_subtree_weights_map, heuristic_info->first_subtree_weights, &index);

    heuristic_info->second_nodes = second_tree->nodes;
    heuristic_info->second_subtree_weights = calloc(second_tree->nodes, sizeof(UINT64));
    heuristic_info->second_subtree_weights_map = g_hash_table_new(g_direct_hash, g_direct_equal);

    index = 0;
    collectSubtreeInfoAux(second_tree->root, heuristic_info->second_subtree_weights_map, heuristic_info->second_subtree_weights, &index);

    return heuristic_info;
}

/*
static compare_heuristic_info_t* collectHeuristicInfoSeparately(compare_heuristic_info_t* info, hcct_tree_t* tree) {
    UINT32 index = 0;
    if (info == NULL) { // will process first_tree only
        compare_heuristic_info_t* heuristic_info = calloc(1, sizeof(compare_heuristic_info_t));
        heuristic_info->first_nodes = tree->nodes;
        collectSubtreeInfoAux(tree->root, heuristic_info->first_subtree_weights_map, heuristic_info->first_subtree_weights, &index);
        return heuristic_info;
    } else { // will process second_tree and update the existing info structure
        compare_heuristic_info_t* heuristic_info = info;
        heuristic_info->second_nodes = tree->nodes;
        heuristic_info->second_subtree_weights = calloc(tree->nodes, sizeof(UINT64));
        heuristic_info->second_subtree_weights_map = g_hash_table_new(g_direct_hash, g_direct_equal);
        collectSubtreeInfoAux(tree->root, heuristic_info->second_subtree_weights_map, heuristic_info->second_subtree_weights, &index);
        return info;
    }
}
*/

static void freeHeuristicInfo(compare_heuristic_info_t* heuristic_info) {
    free(heuristic_info->first_subtree_weights);

    g_hash_table_destroy(heuristic_info->second_subtree_weights_map);
    free(heuristic_info->second_subtree_weights);

    free(heuristic_info);
}

// be careful: for efficiency it performs side-effects on score_matrix and eitherfirst_children_array or second_children_array
static UINT32 bestEffortMatching(double* score_matrix, double* heuristic_matrix, compare_heuristic_info_t* heuristic_info, double adjust_factor,
                                hcct_node_t** first_children_array, UINT64* first_buffer, UINT32 first_children,
                                hcct_node_t** second_children_array, UINT64* second_buffer, UINT32 second_children) {
    #define SCORE(i,j)          (score_matrix[(i)*second_children+(j)])
    #define HEURISTIC(i,j)      (heuristic_matrix[(i)*second_children+(j)])
    UINT32 i, j, k;

    // compute scores for heuristic
    #define GET_SUBTREE_WEIGHT_FIRST(node)      (((compare_info_t*)(node)->extra_info)->subtree_weight)
    #define GET_SUBTREE_WEIGHT_SECOND(node)     (*((UINT64*)g_hash_table_lookup(heuristic_info->second_subtree_weights_map, (node))))

    for (i = 0; i < first_children; ++i) {
        first_buffer[i] = GET_SUBTREE_WEIGHT_FIRST(first_children_array[i]);
    }
    for (j = 0; j < second_children; ++j) {
        second_buffer[j] = adjust_factor * GET_SUBTREE_WEIGHT_SECOND(second_children_array[j]);
    }

    // don't forget to set all valid HEURISTIC(i,j) cells!
    for (i = 0; i < first_children; ++i) {
        for (j = 0; j < second_children; ++j) {
                UINT64 diff = ABSDIFF(first_buffer[i], second_buffer[j]);
                if (diff < first_buffer[i]) {
                    HEURISTIC(i,j) = (1.0 - diff/(double)first_buffer[i]) * WEIGHT_SUBTREE_SUM;
                } else {
                    HEURISTIC(i,j) = 0;
                }
        }
    }

    UINT32 matched = 0;
    if (first_children <= second_children) {
        // try to maximize score for each child of the first node
        for (i = 0; i < first_children; ++i) {
            k = -1;
            double max = 0;
            double tmp;
            for (j = 0; j < second_children; ++j) {
                tmp = SCORE(i,j) + HEURISTIC(i,j);
                if (tmp > max && second_children_array[j] != NULL) {
                    max = tmp;
                    k = j;
                    if (max == 1.0 + WEIGHT_SUBTREE_SUM) break;
                }
            }
            if (k != -1) {
                compare_info_t* cmp_info = (compare_info_t*)first_children_array[i]->extra_info;
                cmp_info->matched_node = second_children_array[k];
                cmp_info->status = STATUS_APX_MATCHED;
                cmp_info->score = SCORE(i,k); // don't include score from heuristic
                second_children_array[k] = NULL; // side-effect (mark it as taken)
                ++matched;
            } else {
                setSubtreeAsNotMatched(first_children_array[i]); // likely unusual
            }
        }
    } else {
        // try to maximize score for each child of the second node
        for (j = 0; j < second_children; ++j) {
            k = -1;
            double max = 0;
            double tmp;
            for (i = 0; i < first_children; ++i) {
                tmp = SCORE(i,j) + HEURISTIC(i,j);
                if (tmp > max && first_children_array[i] != NULL) {
                    max = tmp;
                    k = i;
                    if (max == 1.0 + WEIGHT_SUBTREE_SUM) break;
                }
            }
            if (k != -1) {
                compare_info_t* cmp_info = (compare_info_t*)first_children_array[k]->extra_info;
                cmp_info->matched_node = second_children_array[j];
                cmp_info->status = STATUS_APX_MATCHED;
                cmp_info->score = SCORE(k,j); // don't include score from heuristic
                first_children_array[k] = NULL; // side-effect (mark it as taken)
                ++matched;
            }
        }

        // some nodes will be necessarily remain unmatched
        for (i = 0; i < first_children; ++i) {
            if (first_children_array[i] != NULL) {
                setSubtreeAsNotMatched(first_children_array[i]);
            }
        }
    }
    #undef SCORE
    #undef HEURISTIC
    return matched;
}

static void matchTrees(hcct_tree_t* first_tree, hcct_tree_t* second_tree, size_t heuristic_threshold, compare_heuristic_info_t* heuristic_info) {
    UINT32 i, j;

    // initialize compare info ('status' and 'matched' fields only!)
    resetCompareInfo(first_tree->root);

    // find max degree for both trees and pre-allocate buffers
    UINT32 first_max_degree = getMaxDegree(first_tree);
    UINT32 second_max_degree = getMaxDegree(second_tree);
    printf("[avg degree for the trees: %.2f %.2f]\n", getAvgDegree(first_tree), getAvgDegree(second_tree));
    printf("[max degree for the trees: %lu %lu]\n", first_max_degree, second_max_degree);

    double adjust_factor = getAdjustFactor(first_tree, second_tree);
    printf("[adjust factor for counters: %f]\n", adjust_factor);

    double* score_matrix = malloc(sizeof(double) * first_max_degree * second_max_degree);
    // row-major or column-major shouldn't make much difference for performance
    #define SCORE(i,j)       (score_matrix[(i)*second_children+(j)])

    hcct_node_t** first_children_array = malloc(sizeof(hcct_node_t*) * first_max_degree);
    hcct_node_t** second_children_array = malloc(sizeof(hcct_node_t*) * second_max_degree);
    UINT32* numbers_first = malloc(sizeof(UINT32) * first_max_degree);
    UINT32* numbers_first_bkp = malloc(sizeof(UINT32) * first_max_degree);
    UINT32* numbers_second = malloc(sizeof(UINT32) * second_max_degree);
    UINT32* numbers_second_bkp = malloc(sizeof(UINT32) * second_max_degree);

    double* heuristic_matrix = malloc(sizeof(double) * first_max_degree * second_max_degree);
    UINT64* heuristic_first_buffer = malloc(sizeof(UINT64) * first_max_degree);
    UINT64* heuristic_second_buffer = malloc(sizeof(UINT64) * second_max_degree);

    for (i = 0; i < first_max_degree; ++i) {
        numbers_first_bkp[i] = i;
    }
    for (j = 0; j < second_max_degree; ++j) {
        numbers_second_bkp[j] = j;
    }

    compare_info_t* current_cmp_info = (compare_info_t*)first_tree->root->extra_info;
    current_cmp_info->matched_node = second_tree->root;
    current_cmp_info->status = STATUS_MATCHED;
    current_cmp_info->score = 1.0;

    /* Perform a visit in BFS order using a circular queue */
    #define QUEUE_SIZE  (128*1024) // don't forget parentheses!
    hcct_node_t * queue[QUEUE_SIZE];
    int queue_read_index = QUEUE_SIZE - 1;
    int queue_insert_index = 0;
    int numElems = 1;
    queue[0] = first_tree->root;

    #define ENQUEUE(item)   { queue_insert_index = (queue_insert_index + 1) % QUEUE_SIZE; queue[queue_insert_index] = (item); numElems++; }
    #define DEQUEUE(dest)   { queue_read_index = (queue_read_index + 1) % QUEUE_SIZE; (dest) = queue[queue_read_index]; numElems--; }

    UINT32 progress = 1;
    UINT32 apx_matched = 0;

    while (numElems > 0) {
        if ((progress % MATCH_PROGRESS_INDICATOR) == 0) {
            printf("\r%lu nodes matched so far... (%lu with greedy algorithm)", progress, apx_matched);
            fflush(0);
        }

        if (numElems > QUEUE_SIZE) {
            printf("\n[ERROR] Queue overflow! Try with a bigger buffer (current size is %d).\n", QUEUE_SIZE);
            exit(1);
        }

        hcct_node_t *first, *second, *tmp;

        DEQUEUE(first)
        second = ((compare_info_t*) first->extra_info)->matched_node; // this will never be NULL (I won't enqueue an unmatched node)

        UINT32 first_children = 0, second_children = 0;

        // copy pointers into an array for indexed access
        for (tmp = first->first_child; tmp != NULL; tmp = tmp->next_sibling)
            first_children_array[first_children++] = tmp;
        for (tmp = second->first_child; tmp != NULL; tmp = tmp->next_sibling)
            second_children_array[second_children++] = tmp;

        // compute similarity matrix
        for (i = 0; i < first_children; ++i)
            for (j = 0; j < second_children; ++j)
                SCORE(i,j) = compareNodesLocally(first_children_array[i], second_children_array[j], adjust_factor);

        /* Computing the exact solution requires exponential time, so when the
         * required k-permutations are too many we will approximate the result.
         */
        #ifndef MAX // glib.h
        #define MAX(x,y)        ((x)>(y) ? (x) : (y))
        #endif
        if (MAX(first_children, second_children) > heuristic_threshold) {
            UINT32 ret = bestEffortMatching(score_matrix, heuristic_matrix, heuristic_info, adjust_factor,
                                            first_children_array, heuristic_first_buffer, first_children,
                                            second_children_array, heuristic_second_buffer, second_children);
            progress += ret;
            apx_matched += ret;
            hcct_node_t* this_child;
            UINT32 detected = 0;
            for (this_child = first->first_child; this_child != NULL; this_child = this_child->next_sibling) {
                current_cmp_info = (compare_info_t*)this_child->extra_info;
                if (current_cmp_info->matched_node != NULL) {
                    ++detected;
                    if (this_child->first_child != NULL) { // do not enqueue leaves!
                        if (current_cmp_info->matched_node->first_child != NULL) {
                            ENQUEUE(this_child);
                        } else {
                            // I won't enqueue this_child as its children can't be matched to any node
                            hcct_node_t* tmp;
                            for (tmp = this_child->first_child; tmp != NULL; tmp = tmp->next_sibling)
                                setSubtreeAsNotMatched(tmp);
                        }
                    }
                } // else: bestEffortMatching() already set it as unmatched
            }
            if (detected != ret) {
                printf("\n\nMatched: %lu detected %lu\n", ret, detected);
                exit(1);
            }
            continue;
        }

        if (first_children <= second_children) {
            /* This case is simpler. If the first node has N children, I have to
             * generate all the possible N-permutations from the set of children
             * of the second node.
             */
            UINT32 (*comb_function)(UINT32*, UINT32, UINT32) = &next_k_permutation;
            if (first_children == 1) {
                // dirty workaround (next_k_permutation doesn't work for k=1)
                comb_function = &next_combination;
            }

            // restore numbers from backup
            memcpy(numbers_second, numbers_second_bkp, second_children * sizeof(UINT32));
            int perm_index = -1, max_index = 0;
            double max = 0;
            do {
                perm_index++;
                double ret = 0;
                for (i = 0; i < first_children; ++i) {
                    ret += SCORE(i, numbers_second[i]);
                }
                if (ret == first_children) { // perfect matching
                    max = ret;
                    break;
                } else if (ret > max) {
                    max = ret;
                    max_index = perm_index;
                }
            } while ((*comb_function)(numbers_second, second_children, first_children));

            if (max > 0) {
                if (max != first_children) { // approximate matching
                    // reconstruct the permutation
                    memcpy(numbers_second, numbers_second_bkp, second_children * sizeof(UINT32));
                    for (i = 0; i < max_index; ++i)
                        (*comb_function)(numbers_second, second_children, first_children);
                }

                // set matched nodes (only those with score > 0)
                for (i = 0; i < first_children; ++i) {
                    hcct_node_t* this_child = first_children_array[i];
                    if (SCORE(i, numbers_second[i]) > 0) {
                        ++progress;
                        current_cmp_info = (compare_info_t*)this_child->extra_info;
                        current_cmp_info->matched_node = second_children_array[numbers_second[i]];
                        current_cmp_info->score = SCORE(i, numbers_second[i]);
                        current_cmp_info->status = STATUS_MATCHED;
                        // do not enqueue leaves!
                        if (this_child->first_child != NULL) {
                            if (current_cmp_info->matched_node->first_child != NULL) {
                                ENQUEUE(this_child)
                            } else {
                                // I won't enqueue this_child as its children can't be matched to any node
                                hcct_node_t* tmp;
                                for (tmp = this_child->first_child; tmp != NULL; tmp = tmp->next_sibling)
                                    setSubtreeAsNotMatched(tmp);
                            }
                        }
                    } else {
                        setSubtreeAsNotMatched(this_child);
                    }
                }
            } else {
                // couldn't match any node
                for (i = 0; i < first_children; ++i) {
                    setSubtreeAsNotMatched(first_children_array[i]);
                }
            }
        } else {
            /* This case is more complicated. If the second node has M children,
             * I have to generate all the possible combinations of size M from
             * the set of children of the first node, and compare each of them
             * against all the possible permutations from the set of children of
             * the second node. However, since similarity is symmetric, I can
             * keep things simpler by generating all the M-permutations from the
             * set of children of the first node! This code is very similar to
             * the one in the other branch except for the beginning and the end.
             */

            UINT32 (*comb_function)(UINT32*, UINT32, UINT32) = &next_k_permutation;
            if (second_children == 1) {
                // dirty workaround (next_k_permutation doesn't work for k=1)
                comb_function = &next_combination;
            }

            // restore numbers from backup
            memcpy(numbers_first, numbers_first_bkp, first_children * sizeof(UINT32));
            int perm_index = -1, max_index = 0;
            double max = 0;
            do {
                perm_index++;
                double ret = 0;
                for (j = 0; j < second_children; ++j) {
                    ret += SCORE(numbers_first[j], j); // inverted wrt first branch
                }
                if (ret == second_children) {
                    max = ret;
                    break;
                } else if (ret > max) {
                    max = ret;
                    max_index = perm_index;
                }
            } while ((*comb_function)(numbers_first, first_children, second_children));

            if (max > 0) {
                if (max != second_children) {
                    // reconstruct the permutation
                    memcpy(numbers_first, numbers_first_bkp, first_children * sizeof(UINT32));
                    for (i = 0; i < max_index; ++i)
                        (*comb_function)(numbers_first, first_children, second_children);
                }

                // set matched nodes (only those with score > 0)
                for (j = 0; j < second_children; ++j) {
                    if (SCORE(numbers_first[j], j) > 0) {
                        ++progress;
                        hcct_node_t* this_child = first_children_array[numbers_first[j]];
                        current_cmp_info = (compare_info_t*)this_child->extra_info;
                        current_cmp_info->matched_node = second_children_array[j];
                        current_cmp_info->score = SCORE(numbers_first[j], j);
                        current_cmp_info->status = STATUS_MATCHED;
                        // do not enqueue leaves!
                        if (this_child->first_child != NULL) {
                            if (current_cmp_info->matched_node->first_child != NULL) {
                                ENQUEUE(this_child);
                            } else {
                                // I won't enqueue this_child as its children can't be matched to any node
                                hcct_node_t* tmp;
                                for (tmp = this_child->first_child; tmp != NULL; tmp = tmp->next_sibling)
                                    setSubtreeAsNotMatched(tmp);
                            }
                        }
                    }
                }

                // set remaining nodes as not matched
                for (i = 0; i < first_children; ++i) {
                    current_cmp_info = (compare_info_t*)first_children_array[i]->extra_info;
                    if (current_cmp_info->status != STATUS_MATCHED)
                        setSubtreeAsNotMatched(first_children_array[i]);
                }
            } else {
                // couldn't match any node
                for (i = 0; i < first_children; ++i) {
                    setSubtreeAsNotMatched(first_children_array[i]);
                }
            }
        }
    }
    #undef SCORE
    #undef DEQUEUE
    #undef ENQUEUE
    #undef QUEUE_SIZE

    printf("\r%lu nodes have been matched! %lu required using a greedy algorithm.\n", progress, apx_matched);

    free(score_matrix);
    free(heuristic_matrix);
    free(first_children_array);
    free(second_children_array);
    free(numbers_first);
    free(numbers_first_bkp);
    free(numbers_second);
    free(numbers_second_bkp);
    free(heuristic_first_buffer);
    free(heuristic_second_buffer);
}

/* // check for identity between matched trees (for debugging purposes only)
static void debugForIndentity(hcct_node_t* node) {
    compare_info_t* info = (compare_info_t*) node->extra_info;

    // note that it does not take unmatched nodes into account
    if (info->matched_node != NULL && node->node_id != info->matched_node->node_id) {
        printf("\nMISMATCH DETECTED!\n");
        printf("Current score: %.2f\n", info->score);
        printf("Addresses: %lu %lu\n", (ADDRINT) node, (ADDRINT) info->matched_node);
        printf("Counters: %lu %lu\n", node->counter, info->matched_node->counter);
        printf("Routine 1st: %s %s %s %lu\n", node->routine_sym->name, node->routine_sym->file, node->routine_sym->image, node->routine_sym->offset);
        printf("Routine 2nd: %s %s %s %lu\n", info->matched_node->routine_sym->name, info->matched_node->routine_sym->file, info->matched_node->routine_sym->image, info->matched_node->routine_sym->offset);
        printf("Call site 1st: %s %s %s %lu\n", node->call_site_sym->name, node->call_site_sym->file, node->call_site_sym->image, node->call_site_sym->offset);
        printf("Call site 2nd: %s %s %s %lu\n", info->matched_node->call_site_sym->name, info->matched_node->call_site_sym->file, info->matched_node->call_site_sym->image, info->matched_node->call_site_sym->offset);
        return;
    }

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        debugForIndentity(child);
}
*/

static void allocCompareInfo(hcct_node_t* node) {
    node->extra_info = calloc(1, sizeof (compare_info_t));

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        allocCompareInfo(child);
}

static void freeCompareInfo(hcct_node_t* node) {
    free(node->extra_info);
    node->extra_info = NULL;

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        freeCompareInfo(child);
}

static int parseArgMapsOrSymsFile(char* orig, char** maps_file, char** syms_file) {
    size_t len = strlen(orig);
    if (len > 4) { // I won't do this accurately :-)
        if (!strcmp(orig+len-4, ".map") || !strcmp(orig+len-5, ".maps")) {
            *maps_file = orig;
            return 0;
        } else if (!strcmp(orig+len-4, ".sym") || !strcmp(orig+len-5, ".syms")) {
            *syms_file = orig;
            return 0;
        }
    }
    return -1;
}

int main(int argc, char** argv) {
    // internal parameters
    double match_thresholds[] = {1.0, 0.95, 0.9, 0.85, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.05};

    hcct_tree_t *tree_1, *tree_2;
    int full_mode = 1;
    double phi = 0;
    char *program = NULL;
    char *maps_file_1 = NULL, *maps_file_2 = NULL;
    char *syms_file_1 = NULL, *syms_file_2 = NULL;
    syms_map_t *syms_map_1 = NULL, *syms_map_2 = NULL; // I will use a helper function
    compare_heuristic_info_t* heuristic_info = NULL;

    if (argc != 7 && argc != 8) {
        printf("This tool tries to match all nodes from the first tree with nodes from the other, then compares them.\n\n");

        //printf("Syntax: %s <logfile_1> <mapfile_1> <logfile_2> <mapfile_2> <PHI> <adjust_parameter> <likelihood_threshold>\n\n", argv[0]);
        printf("Syntax:\n%s <logfile_1> <maps_file_1> <logfile_2> <maps_file_2> <program> <PHI> [full | quick]\n\n", argv[0]);
        printf("or: %s <logfile_1> <syms_file_1> <logfile_2> <syms_file_2> <program> <PHI> [full | quick]\n", argv[0]);
        if (argc < 7) {
            printf("[ERROR] Too few arguments!\n");
        } else {
            printf("[ERROR] Too many arguments!\n");
        }
        exit(1);
    }

    if (parseArgMapsOrSymsFile(argv[2], &maps_file_1, &syms_file_1)) {
        printf("[ERROR] File %s is not a valid .map, .maps, .sym or .syms file!\n", argv[2]);
        exit(1);
    }
    if (parseArgMapsOrSymsFile(argv[4], &maps_file_2, &syms_file_2)) {
        printf("[ERROR] File %s is not a valid .map, .maps, .sym or .syms file!\n", argv[4]);
        exit(1);
    }

    // parse phi parameter from command line
    char* end;
    double d = strtod(argv[6], &end);
    // *end: see http://stackoverflow.com/questions/5580975/problem-with-string-conversion-to-number-strtod
    if (*end != 0 || d <= 0 || d >= 1.0) { // 0 is an error code
        printf("[WARNING] Invalid value specified for phi, using default instead\n");
    } else phi = d;

    program = argv[5]; // TODO: qualche check (se lo ometto, phi scala di 1 e salta tutto il resto)

    if (argc == 8) {
        if (strcmp(argv[7], "quick")) {
            if (strcmp(argv[7], "full")) {
                printf("[WARNING] Invalid mode specified, using default (full) instead\n");
            }
        } else {
            full_mode = 0;
        }
    }

    // build the two trees in memory
    printf("#################\n");
    printf("Loading tree 1...\n");
    printf("#################\n\n");
    tree_1 = loadTree(argv[1]);
    if (tree_1 == NULL) {
        printf("[ERROR] couldn't load tree from %s", argv[1]);
        exit(1);
    }

    // in quick mode the goal is to use as less memory as possible (tree_1 is likely to be huge)
    if (!full_mode) {
        /* Pruning tree now can degrade quality when apx matching is used! */
        pruneTreeAboveCounter(tree_1, getMinThreshold(tree_1, getPhi(tree_1, phi)));
        printf("\nNodes in the first tree after pruning: %lu\n", countNodes(tree_1->root));
    }

    printf("\n#################\n");
    printf("Loading tree 2...\n");
    printf("#################\n\n");
    tree_2 = loadTree(argv[3]);
    if (tree_2 == NULL) {
        printf("[ERROR] couldn't load tree from %s", argv[3]);
        exit(1);
    }

    // allocate extra fields for comparing trees
    printf("\nAllocating extra fields...\n");
    allocCompareInfo(tree_1->root);

    if (full_mode) {
        printf("\nCollecting heuristic information before any tree pruning...\n");
        heuristic_info = collectHeuristicInfo(tree_1, tree_2);
        printf("Max weight for first tree:  %llu\n", ((compare_info_t*)tree_1->root->extra_info)->subtree_weight);
        printf("Max weight for second tree: %llu\n", heuristic_info->second_subtree_weights[tree_2->nodes-1]);

        if (tree_2->tool == LSS_FULL || tree_2->tool == LSS_BURST || tree_2->tool == LSS_TIME) {
            pruneTreeAboveCounter(tree_2, getMinThreshold(tree_2, getPhi(tree_2, phi)));
            printf("\nNodes in the second tree after pruning: %lu\n", countNodes(tree_2->root));
        }
    } else {
        pruneTreeAboveCounter(tree_2, getMinThreshold(tree_2, getPhi(tree_2, phi)));
        printf("\nNodes in the second tree after pruning: %lu\n", countNodes(tree_2->root));

        printf("\nCollecting heuristic information after both trees have been pruned...\n");
        heuristic_info = collectHeuristicInfo(tree_1, tree_2);
        //heuristic_info = collectHeuristicInfoSeparately(NULL, tree_1);
        printf("Max weight for first tree:  %llu\n", ((compare_info_t*)tree_1->root->extra_info)->subtree_weight);
        //collectHeuristicInfoSeparately(heuristic_info, tree_2);
        printf("Max weight for second tree: %llu\n", heuristic_info->second_subtree_weights[tree_2->nodes-1]);
    }

    // solve symbols
    printf("\nSolving symbols for the first tree...\n");
    solveTree(tree_1, program, maps_file_1, syms_file_1, &syms_map_1);

    printf("\nSolving symbols for the second tree...\n");
    solveTree(tree_2, program, maps_file_2, syms_file_2, &syms_map_2);

    // this can speed up the matching process when the avg/max degree is large
    printf("\nReordering nodes in the first tree by decreasing frequency...\n");
    sortChildrenByDecreasingFrequency(tree_1);
    printf("Reordering nodes in the second tree by decreasing frequency...\n");
    sortChildrenByDecreasingFrequency(tree_2);

    // try to match nodes from tree 1 with nodes from tree 2

    if (full_mode) {
        printf("\nMatching nodes now...\n");
        matchTrees(tree_1, tree_2, HEURISTIC_THRESHOLD_FULL, heuristic_info);
        printf("\n");
        printMatchingStats(tree_1, tree_2, match_thresholds, sizeof(match_thresholds)/sizeof(double));

        // these statistics can't be computed if tree_1 has been pruned
        printUncoveredStats(tree_1);
        printHotEdgeCoverageStats(tree_1, phi);
    } else {
        printf("Matching nodes now...\n");
        matchTrees(tree_1, tree_2, HEURISTIC_THRESHOLD_QUICK, heuristic_info);
        printf("\n");
        printMatchingStats(tree_1, tree_2, match_thresholds, sizeof(match_thresholds)/sizeof(double));
    }

    printAccuracyStats(tree_1, tree_2, phi);

    // be careful: they will need to prune tree_1
    printFalseColdHotStats(tree_1, tree_2, phi, full_mode);

    // clean up
    freeHeuristicInfo(heuristic_info);
    freeCompareInfo(tree_1->root); // note that freeTree() would try to free extra_info
    freeSymbols(syms_map_1);
    freeTree(tree_1);
    freeSymbols(syms_map_2);
    freeTree(tree_2);

    return 0;

    // TODO: controllare quando non ho info su simboli
    // TODO: controllare [home] per mappe simboli nella stampa
}