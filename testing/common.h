#define UINT32 unsigned long
#define UINT16 unsigned short

#define STACK_MAX_DEPTH	1024
#define PAGE_SIZE		1024
#define SHOW_MESSAGES	0

#if SHOW_MESSAGES==1
// Edit here
#define DUMP_STATS		1
#define DUMP_TREE		0
#else
// No messages shown on screen
#define DUMP_STATS		0
#define DUMP_TREE		0
#endif
