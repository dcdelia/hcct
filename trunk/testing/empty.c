#include <stdlib.h>
#include "empty.h"

void* hcct_get_root() {
    return NULL;
}

void hcct_init() {
}

#if PROFILE_TIME==0
void hcct_enter(ADDRINT routine_id, ADDRINT call_site) {
}
#else
void hcct_enter(ADDRINT routine_id, ADDRINT call_site, UINT32 increment) {
}
#endif

void hcct_exit() {
}

void hcct_dump() {
}

#if BURSTING==1
void hcct_align() {
}
#elif PROFILE_TIME==1
void hcct_align(UINT32 increment) {
}
#endif
