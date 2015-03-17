#ifndef __OUTPUT__
#define __OUTPUT__

#include <stdio.h>

#include "common.h"

void printTree(hcct_tree_t* tree, char*(*print)(hcct_node_t*, hcct_tree_t*));
void printGraphviz(hcct_tree_t* tree, FILE* out, char*(*print)(hcct_node_t*, hcct_tree_t*));
void printJSON(hcct_node_t* node, FILE* out);

#endif

