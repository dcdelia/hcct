#ifndef __CONFIG__
#define __CONFIG__

// 32 bit - x86
#define ADDRINT   unsigned long
#define UINT16    unsigned short
#define UINT32    unsigned long
#define UINT64    unsigned long long

// general configuration parameters
#define DEFAULT_DUMP_PATH "/tmp"
#define STACK_MAX_DEPTH   1024
#define PAGE_SIZE     1024
#define BUFLEN        512
#define SHOW_MESSAGES   0
#define DUMP_TREE     1

// LSS defaults
#define INLINE_UPD_MIN          1
#define UPDATE_MIN_SENTINEL     1
#define KEEP_EPS        0
#define EPSILON         50000 // also used in analysis.c

#if BURSTING==1
// bursting defaults
#define UPDATE_ALONG_TREE   0

/* Original settings */
//#define TIMER_TYPE      CLOCK_PROCESS_CPUTIME_ID
//#define SAMPLING_INTERVAL   2000000
//#define BURST_LENGTH        200000

/* Different timer */
//#define TIMER_TYPE      CLOCK_REALTIME
//#define SAMPLING_INTERVAL   2000000
//#define BURST_LENGTH        200000

/* Different granularity - faster!*/
#define TIMER_TYPE      CLOCK_PROCESS_CPUTIME_ID
#define SAMPLING_INTERVAL   20000000
#define BURST_LENGTH        2000000

#elif PROFILE_TIME==1
// defaults for execution time metrics
#define TIMER_TYPE      CLOCK_REALTIME
#define SAMPLING_INTERVAL 10000 // 0.01 ms
#endif
  
// shadow stack
typedef struct hcct_stack_node_s hcct_stack_node_t;
struct hcct_stack_node_s {
  ADDRINT   routine_id;    
  ADDRINT   call_site;
};
    
#endif
