#ifndef __SYMBOLS__
#define __SYMBOLS__

#include "analysis.h"

hcct_map_t* createMemoryMap(char* program);
void freeMemoryMap(hcct_map_t* map);

hcct_sym_t* getFunctionFromAddress(ADDRINT addr, hcct_tree_t* tree, hcct_map_t* map);
void freeSym(hcct_sym_t* sym);

#endif
