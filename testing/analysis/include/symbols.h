#ifndef __SYMBOLS__
#define __SYMBOLS__

#include <stdio.h>
#include <glib.h>

typedef struct sym_s sym_t;
typedef struct syms_map_s syms_map_t;
typedef struct regions_list_s regions_list_t;

#include "common.h"

#define SYM_UNKNOWN             "UNKNOWN" // for addresses not solved at all
#define SYM_UNRESOLVED_NAME     "UNRSLVD"
#define SYM_UNRESOLVED_FILE     "UNRSLVD"
#define SYM_UNRESOLVED_IMAGE    "UNRSLVD"

struct sym_s {
    char* name; // addr2line or <unknown> or address
    char* file; // addr2line or range
    char* image; // from /proc/self/maps or program_short_name
    ADDRINT offset; // from /proc/self/maps (0 if no map available)
};

struct syms_map_s {
    GHashTable* addresses; // keys are addresses, values are pointers to hcct_sym_t objects
    char* program;
    char* mapfile;
    regions_list_t* regions_list;
};

struct regions_list_s {
    ADDRINT start;
    ADDRINT end;
    char* pathname;
    ADDRINT offset; // currently unused
    regions_list_t* next;
};


syms_map_t* readMapFromFile(FILE* file);
syms_map_t* initializeSymbols(char* program, char* mapfile);
void manuallyAddSymbol(syms_map_t* syms_map, ADDRINT address, sym_t* sym);
sym_t* getSolvedSymbol(ADDRINT addr, syms_map_t* sym_map);
sym_t* solveAddress(ADDRINT addr, syms_map_t* sym_map);
void dumpMapToFile(syms_map_t* sym_map, FILE* out);
void freeSymbols(syms_map_t* sym_map);
int compareSymbols(sym_t* first, sym_t* second);


#endif
