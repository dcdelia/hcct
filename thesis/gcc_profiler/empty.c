#include <stdlib.h>
#include "empty.h"

void* hcct_get_root() {
    return NULL;
}

int hcct_getenv() {
    return 0;
}

int hcct_init() {
    return 0;
}

void hcct_enter(UINT32 routine_id, UINT16 call_site) {
}

void hcct_exit() {
}

void hcct_dump() {
}

#if BURSTING==1
void hcct_align() {
}
#endif
