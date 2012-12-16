#ifndef __EMPTY__
#define __EMPTY__

#include "config.h"

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
