#ifndef __COMMON__
#define __COMMON__

#define UINT32 unsigned long
#define UINT16 unsigned short

#define STACK_MAX_DEPTH	1024
#define PAGE_SIZE		1024
#define SHOW_MESSAGES   1
#define DUMP_STATS		0
#define DUMP_TREE		1

#if BURSTING==1
#define SAMPLING_INTERVAL   10*1000000
#define BURST_LENGTH        1*1000000
#define UPDATE_ALONG_TREE   0

// Shadow stack hcct_stack_node_t[]
typedef struct hcct_stack_node_s hcct_stack_node_t;
struct hcct_stack_node_s {
    UINT32           routine_id;    
    UINT16           call_site;
};
#endif

#endif
