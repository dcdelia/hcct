#ifndef __CCT__
#define __CCT__

#include "common.h"

typedef struct cct_node_s cct_node_t;
struct cct_node_s {
    ADDRINT     routine_id;    
    ADDRINT     call_site;
    UINT32      counter;
    cct_node_t* first_child;
    cct_node_t* next_sibling;    
};

void hcct_init() __attribute__((no_instrument_function));
#if PROFILE_TIME==0
void hcct_enter(ADDRINT routine_id, ADDRINT call_site) __attribute__((no_instrument_function));
#else
void hcct_enter(ADDRINT routine_id, ADDRINT call_site, UINT32 increment) __attribute__((no_instrument_function));
#endif
void hcct_exit() __attribute__((no_instrument_function));
void hcct_dump() __attribute__((no_instrument_function));

#if BURSTING==1
void hcct_align() __attribute__((no_instrument_function));
#elif PROFILE_TIME==1
void hcct_align(UINT32 increment) __attribute__((no_instrument_function));
#endif

#endif
