#ifndef __LSS__
#define __LSS__

#include "common.h"

#define INLINE_UPD_MIN          1
#define UPDATE_MIN_SENTINEL     1
#define KEEP_EPS				0
#define PHI						1000
#define EPSILON					10000

typedef struct lss_hcct_node_s lss_hcct_node_t;
struct lss_hcct_node_s {
    UINT32           routine_id;    
    UINT32           counter;    
    lss_hcct_node_t* first_child;
    lss_hcct_node_t* next_sibling;
    UINT32           call_site;
    UINT16           additional_info;
    lss_hcct_node_t* parent;
    #if KEEP_EPS
    int              eps;    
    #endif
};

int hcct_getenv() __attribute__((no_instrument_function));
int hcct_init() __attribute__((no_instrument_function));
void hcct_enter(UINT32 routine_id, UINT32 call_site) __attribute__((no_instrument_function));
void hcct_exit() __attribute__((no_instrument_function));
void hcct_dump() __attribute__((no_instrument_function));

lss_hcct_node_t* hcct_get_root() __attribute__((no_instrument_function));

#if BURSTING==1
void hcct_align() __attribute__((no_instrument_function));
#endif

#endif
