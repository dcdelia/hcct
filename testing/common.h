#ifndef __COMMON__
#define __COMMON__

// 32 bit - x86
#define ADDRINT	unsigned long
#define UINT64 unsigned long long
#define UINT32 unsigned long
#define UINT16 unsigned short

// general configuration parameters
#define DEFAULT_EPSILON		10000
#define PROFILE_TIME_INTVL	10000 // 0.01 ms
#define STACK_MAX_DEPTH		1024
#define PAGE_SIZE			1024
#define BUFLEN				512
#define SHOW_MESSAGES		0
#define DUMP_TREE			1

// LSS defaults
#define INLINE_UPD_MIN          1
#define UPDATE_MIN_SENTINEL     1
#define KEEP_EPS				0
#define EPSILON					10000 // used also in analysis.c

#if BURSTING==1
// static bursting defaults
#define SAMPLING_INTERVAL   10*1000000
#define BURST_LENGTH        1*1000000
#define UPDATE_ALONG_TREE   0
#endif
	
// shadow stack
typedef struct hcct_stack_node_s hcct_stack_node_t;
struct hcct_stack_node_s {
	ADDRINT		routine_id;    
	ADDRINT		call_site;
};
		
#endif
