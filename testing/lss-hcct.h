#ifndef __LSS__
#define __LSS__

#include "config.h"

typedef struct lss_hcct_node_s lss_hcct_node_t;
struct lss_hcct_node_s {
    ADDRINT          routine_id;    
    ADDRINT          call_site;
    UINT32           counter;    
    lss_hcct_node_t* first_child;
    lss_hcct_node_t* next_sibling;    
    UINT16           additional_info;
    lss_hcct_node_t* parent;
    #if KEEP_EPS
    int              eps;    
    #endif
};

void hcct_init() __attribute__((no_instrument_function));
void hcct_exit() __attribute__((no_instrument_function));
void hcct_dump() __attribute__((no_instrument_function));

#if PROFILE_TIME==0
void hcct_enter(ADDRINT routine_id, ADDRINT call_site) __attribute__((no_instrument_function));
#else
void hcct_enter(ADDRINT routine_id, ADDRINT call_site, UINT32 increment) __attribute__((no_instrument_function));
#endif

#if BURSTING==1
void hcct_align() __attribute__((no_instrument_function));
#elif PROFILE_TIME==1
void hcct_align(UINT32 increment) __attribute__((no_instrument_function));
#endif

#endif
