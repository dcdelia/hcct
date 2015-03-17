#ifndef __COMMON__
#define __COMMON__

#include "config.h"
#include <sys/types.h>

#include "symbols.h"
#include "tree.h"

// useful macros
#define ABSDIFF(x,y)    ((x)>(y) ? ((x)-(y)) : ((y)-(x)))

// tool used to create logfile (code)
#define CCT_FULL    1
#define LSS_FULL    2
#define CCT_BURST   3
#define LSS_BURST   4
#define CCT_TIME    5
#define LSS_TIME    6

// node sizes for IA32
#define CCT_NODE_SIZE   20 // two ADDRINT, one UINT32, two pointers
#define LSS_NODE_SIZE   28 // two ADDRINT, one UINT32, three pointers, one UINT16 plus padding

// tool used to create logfile (name)
#define CCT_FULL_STRING         "cct"
#define LSS_FULL_STRING         "lss-hcct"
#define CCT_BURST_STRING        "cct-burst"
#define LSS_BURST_STRING        "lss-hcct-burst"
#define CCT_TIME_STRING         "cct-time"
#define LSS_TIME_STRING         "lss-hcct-time"

#endif
