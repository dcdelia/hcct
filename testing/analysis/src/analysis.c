#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <locale.h>

#include "common.h"
#include "metrics.h"
#include "output.h"
#include "symbols.h"
#include "tree.h"

static UINT64 estimatePoolSpaceUsage(UINT32 nodes, UINT16 block_size) {
    UINT64 num_pages = nodes / PAGE_SIZE;
    if ( nodes % PAGE_SIZE != 0)
        ++num_pages;
    return 12 + num_pages * (4 + (UINT64)PAGE_SIZE * block_size);
}

static int distinctRoutinesAndCallSitesCompare(const void* x, const void* y) {
    // VERY IMPORTANT: array is dynamically allocated!
    hcct_node_t* first = *(hcct_node_t**) x;
    hcct_node_t* second = *(hcct_node_t**) y;

    int ret = first->routine_sym - second->routine_sym;
    if (!ret) {
        ret = first->call_site_sym - second->call_site_sym;
    }
    return ret;
}

static void distinctRoutinesAndCallSitesAux(hcct_node_t* node, hcct_node_t** nodes_array, int* index) {
    nodes_array[(*index)++] = node;

    hcct_node_t* child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        distinctRoutinesAndCallSitesAux(child, nodes_array, index);
}

static void distinctRoutinesAndCallSites(hcct_tree_t* tree, UINT32* routines, UINT32* call_sites) {
    UINT32 nodes = tree->nodes;
    int index = 0;
    hcct_node_t** nodes_array = malloc(nodes * sizeof(hcct_node_t*));
    distinctRoutinesAndCallSitesAux(tree->root, nodes_array, &index);

    if (index != nodes) {
        printf("ERROR: the image of the tree in main memory is corrupted!\n");
        exit(1);
    }

    qsort(nodes_array, nodes, sizeof(hcct_node_t*), distinctRoutinesAndCallSitesCompare);

    *routines = 0; // one is <dummy>
    for (index = 1; index < nodes; ++index) {
        if (nodes_array[index]->routine_sym != nodes_array[index-1]->routine_sym) {
            ++(*routines);
        }
    }

    *call_sites = 0; // one is <dummy>
    for (index = 1; index < nodes; ++index) {
        if (nodes_array[index]->routine_sym != nodes_array[index-1]->routine_sym) {
            ++(*call_sites);
        } else if (nodes_array[index]->call_site_sym != nodes_array[index-1]->call_site_sym) {
            ++(*call_sites);
        }
    }

    free(nodes_array);
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

int main(int argc, char* argv[]) {

    hcct_tree_t *tree;
    syms_map_t* syms_map = NULL;

    char *maps_file = NULL, *syms_file = NULL;
    char *program = NULL, *output_prefix = NULL;

    double phi = 0;

    if (argc != 5 && argc != 6) {
        printf("Syntax: %s <logfile> <maps_file> <program> <output_prefix> [<PHI>]\n", argv[0]);
        printf("or: %s <logfile> <syms_file> <program> <output_prefix> [<PHI>]\n", argv[0]);
        exit(1);
    }

    if (argc == 6) {
        // *end: see http://stackoverflow.com/questions/5580975/problem-with-string-conversion-to-number-strtod
        char* end;
        double d = strtod(argv[5], &end);
        if (*end != 0 || d <= 0 || d >= 1.0) { // 0 is an error code
            printf("[WARNING] Invalid value specified for phi, using default instead\n");
        } else phi = d;
    }

    if (parseArgMapsOrSymsFile(argv[2], &maps_file, &syms_file)) {
        printf("[ERROR] File %s is not a valid .map, .maps, .sym or .syms file!\n", argv[2]);
        exit(1);
    }

    program = argv[3];
    output_prefix = argv[4];

    tree = loadTree(argv[1]);
    if (tree == NULL) exit(1);

    // space usage
    if (tree->tool == CCT_FULL || tree->tool == CCT_BURST || tree->tool == CCT_TIME)
        printf("\nEstimated space used by the allocator: %llu KBs\n", estimatePoolSpaceUsage(tree->nodes, CCT_NODE_SIZE)/1024);
    else if (tree->tool == LSS_FULL || tree->tool == LSS_BURST || tree->tool == LSS_TIME)
        printf("\nEstimated space used by the allocator: %llu KBs\n", estimatePoolSpaceUsage(tree->nodes, LSS_NODE_SIZE)/1024);

    if (tree->tool == CCT_BURST || tree->tool == LSS_BURST)
        printf("\nBefore pruning, tree has %lu nodes and length of sampled stream is %llu.\n", tree->nodes, tree->sampled_enter_events);
    else if (tree->tool == CCT_TIME || tree->tool == LSS_TIME)
        printf("\nBefore pruning, tree has %lu nodes and sum of counters is %llu.\n", tree->nodes, tree->sum_of_tics);
    else // CCT_FULL, LSS_FULL
        printf("\nBefore pruning, tree has %lu nodes and stream length is %llu.\n", tree->nodes, tree->exhaustive_enter_events);

    phi = getPhi(tree, phi);
    printf("\nValue used for phi: %.8f\n", phi);
    tree->phi = phi;

    UINT32 min_counter = getMinThreshold(tree, phi);
    if (tree->tool != CCT_FULL && tree->tool != CCT_BURST && tree->tool != CCT_TIME) { // don't prune a CCT
        pruneTreeAboveCounter(tree, min_counter);
    }

    printf("Number of nodes (hot and cold): %lu\n", tree->nodes);
    UINT32 hot = hotNodes(tree->root, min_counter);
    printf("Number of hot nodes (counter>%lu): %lu\n", min_counter, hot);
    UINT64 sum = sumCounters(tree->root);
    printf("Sum of counters: %llu\n", sum);
    UINT32 hottest = hottestCounter(tree->root);
    printf("Hottest counter: %lu\n", hottest);
    UINT32 closest = largerThanHottest(tree->root, 10, hottest); // TAU=10
    printf("Hottest nodes for TAU=10: %lu\n", closest);

    // solve symbols in the tree
    UINT32 routines, call_sites;
    printf("\nTrying to solve symbols in the tree...\n");
    solveTree(tree, program, maps_file, syms_file, &syms_map);

    printf("\nNumber of distinct addresses solved: %lu\n", (unsigned long)g_hash_table_size(syms_map->addresses));
    distinctRoutinesAndCallSites(tree, &routines, &call_sites); // routines and call sites
    printf("Number of distinct routines: %lu\n", routines);
    printf("Number of distinct call sites: %lu\n\n", call_sites);


#if 0
    // print tree to screen
    printf("\n");
    printTree(tree);
    printf("\n");
#endif

    char buf[BUFLEN+1];
    sprintf(buf, "%s.dot", output_prefix);

    FILE* dot_file;
    dot_file = fopen(buf, "w+");
    if (dot_file == NULL) {
        printf("[ERROR] Cannot create output file %s!\n", buf);
        exit(1);
    }
    printf("Saving HCCT in GraphViz file %s...", buf);
    printGraphviz(tree, dot_file, NULL);
    printf(" done!\n");
    fclose(dot_file);

#define OUTPUT_TREE_EXTRA 0
#if OUTPUT_TREE_EXTRA
    // to be reused from now on!
    char tmp_buf[BUFLEN + 1];

    /* Not required for now
    // create png graph
    sprintf(tmp_buf, "%s-%d.png", tree->short_name, tree->tid);
    sprintf(command, "dot -Tpng %s -o %s &> /dev/null", outgraph_name, tmp_buf);
        if (system(command)!=0) printf("Please check that GraphViz is installed in order to generate PNG graph!\n");
        else printf("PNG graph %s generated successfully!\n", tmp_buf);

        // create svg graph (very useful for large graphs)
        sprintf(tmp_buf, "%s-%d.svg", tree->short_name, tree->tid);
        sprintf(command, "dot -Tsvg %s -o %s &> /dev/null", outgraph_name, tmp_buf);
        if (system(command)!=0) printf("Please check that GraphViz is installed in order to generate SVG graph!\n");
        else printf("SVG graph %s generated successfully!\n", tmp_buf);
     */

    free(outgraph_name);
#endif

#if 0

    /* Not required for now
    // create JSON file for D3
    FILE* outjson;
    sprintf(tmp_buf, "%s-%d.json", tree->short_name, tree->tid);
    printf("Saving HCCT in JSON file %s for D3...", tmp_buf);
    outjson=fopen(tmp_buf, "w+");
    printD3json(tree->root, outjson);
    printf(" done!\n\n");
    fclose(outjson);
     */
#endif

    freeSymbols(syms_map);
    freeTree(tree);

    return 0;
}
