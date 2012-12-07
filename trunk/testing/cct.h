#ifndef __CCT__
#define __CCT__

#include "common.h"

// TODO
#ifndef USE_MALLOC
#define USE_MALLOC 0
#else
#define USE_MALLOC 1
#endif

typedef struct cct_node_s cct_node_t;
struct cct_node_s {
    ADDRINT     routine_id;    
    ADDRINT     call_site;
    UINT32      counter;
    cct_node_t* first_child;
    cct_node_t* next_sibling;    
};

int hcct_getenv() __attribute__((no_instrument_function));
int hcct_init() __attribute__((no_instrument_function));
void hcct_enter(ADDRINT routine_id, ADDRINT call_site) __attribute__((no_instrument_function));
void hcct_exit() __attribute__((no_instrument_function));
void hcct_dump() __attribute__((no_instrument_function));

cct_node_t* hcct_get_root() __attribute__((no_instrument_function));

#if BURSTING==1
void hcct_align() __attribute__((no_instrument_function));
#endif

#endif
