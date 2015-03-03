#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "symbols.h"
#include "tree.h"

static void solveNodes(hcct_node_t* node, syms_map_t* sym_map, UINT32* counter) {
    node->routine_sym = solveAddress(node->routine_id, sym_map);
    node->call_site_sym = solveAddress(node->call_site, sym_map);
    if (( ++(*counter) % 100) == 0) {
        printf("\r%lu symbols solved...", *counter*2);
        fflush(0);
    }
    hcct_node_t *child;
    for (child = node->first_child; child != NULL; child = child->next_sibling)
        solveNodes(child, sym_map, counter);
}

int main(int argc, char* argv[]) {
    char *maps_filename, *program, *output_filename;
    FILE *out;
    UINT32 solved = 0;

    if (argc != 5) {
        printf("Syntax: %s <tree_file> <maps_file> <program> <output_file>\n", argv[0]);
        printf("If no mapfile is available, then use \"none\".\n");
        exit(1);
    }

    maps_filename = argv[2];
    program = argv[3];
    output_filename = argv[4];

    if (access(program, F_OK) != 0) {
        printf("[ERROR] Program %s does not exists, cannot solve symbols!\n", program);
        exit(1);
    }

    hcct_tree_t* tree = loadTree(argv[1]);
    if (tree == NULL) exit(1);

    syms_map_t* sym_map = initializeSymbols(program, maps_filename);
    printf("Number of symbols: %lu\n", tree->nodes*2-2); // root has dummy routine and call site
    hcct_node_t* child;
    for (child = tree->root->first_child; child != NULL; child = child->next_sibling)
        solveNodes(child, sym_map, &solved);
    printf("\r%lu symbols have been solved. %u distinct symbols found.\n", solved*2, g_hash_table_size(sym_map->addresses));
    printf("\n");

    out = fopen(output_filename, "w+");
    if (out == NULL) {
        printf("[ERROR] Cannot create output file %s!\n", output_filename);
        exit(1);
    }
    dumpMapToFile(sym_map, out);
    fclose(out);

    freeSymbols(sym_map); // to trigger clean-up rather than doing cleanSymbols(sym_map)
    freeTree(tree);

    // for debugging purposes only
    out = fopen(output_filename, "r+");
    sym_map = readMapFromFile(out);
    fclose(out);
    freeSymbols(sym_map);

    return 0;
}
