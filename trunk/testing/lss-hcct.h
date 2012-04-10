#ifndef __LSS__
#define __LSS__

#include "common.h"
#include "allocator/pool.h"

#define INLINE_UPD_MIN          1
#define UPDATE_MIN_SENTINEL     1
#define KEEP_EPS				0
#define PHI						10
#define EPSILON					150

typedef struct lss_hcct_node_s lss_hcct_node_t;
struct lss_hcct_node_s {
    UINT32           routine_id;    
    UINT32           counter;    
    lss_hcct_node_t* first_child;
    lss_hcct_node_t* next_sibling;
    UINT16           call_site;
    UINT16           additional_info;
    lss_hcct_node_t* parent;
    #if KEEP_EPS
    int              eps;    
    #endif
};

int hcct_getenv() __attribute__((no_instrument_function));
int hcct_init() __attribute__((no_instrument_function));
void hcct_enter(UINT32 routine_id, UINT16 call_site) __attribute__((no_instrument_function));
void hcct_exit() __attribute__((no_instrument_function));
void hcct_dump() __attribute__((no_instrument_function));

lss_hcct_node_t* hcct_get_root() __attribute__((no_instrument_function));

#if BURSTING==1
void hcct_align() __attribute__((no_instrument_function));
#endif

#endif
