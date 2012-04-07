#include <stdio.h>
#include <stdlib.h>
#include "cct.h"

#if USE_MALLOC==0
	#include "allocator/pool.h"
	#define PAGE_SIZE		1024
	__thread pool_t     *cct_pool;
	__thread void       *cct_free_list;
#endif

// globals
__thread int         cct_stack_idx;
__thread cct_node_t *cct_stack[STACK_MAX_DEPTH];
__thread cct_node_t *cct_root;
__thread UINT32      cct_nodes;

// TODO
extern __thread pid_t hcct_thread_id;

cct_node_t* hcct_get_root()
{
    return cct_root;
}

int hcct_init()
{

    cct_stack_idx   = 0;
    cct_nodes       = 1;
    
    #if USE_MALLOC    
    cct_stack[0]=(cct_node_t*)malloc(sizeof(cct_node_t));
    #else
    // initialize custom memory allocator
    cct_pool = pool_init(PAGE_SIZE, sizeof(cct_node_t), &cct_free_list);
    if (cct_pool == NULL) {
			printf("[hcct] error while initializing allocator... Quitting!\n");
			return -1;
	}

    pool_alloc(cct_pool, cct_free_list, cct_stack[0], cct_node_t);
    if (cct_stack[0] == NULL) {
			printf("[hcct] error while initializing cct root node... Quitting!\n");
			return -1;
	}
	#endif

    cct_stack[0]->first_child = NULL;
    cct_stack[0]->next_sibling = NULL;
    cct_root = cct_stack[0];

    return 0;
}

void hcct_enter(UINT32 routine_id, UINT16 call_site)
{

    cct_node_t *parent=cct_stack[cct_stack_idx++];
    cct_node_t *node;
    for (node=parent->first_child; node!=NULL; node=node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;

    if (node!=NULL) {
        cct_stack[cct_stack_idx]=node;
        node->counter++;
        return;
    }

    ++cct_nodes;
    #if USE_MALLOC
    node=(cct_node_t*)malloc(sizeof(cct_node_t));
    #else
    pool_alloc(cct_pool, cct_free_list, node, cct_node_t);
    #endif
    cct_stack[cct_stack_idx] = node;
    node->routine_id = routine_id;
    node->first_child = NULL;
    node->counter = 1;
    node->call_site =  call_site;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}

void hcct_exit()
{
    cct_stack_idx--;
}

void __attribute__((no_instrument_function)) hcct_dump_aux(cct_node_t* root, int indent, unsigned long *nodes)
{
        if (root==NULL) return;
        
        (*nodes)++;        
        cct_node_t* ptr;

		#if DUMP_TREE==1
		int i;
		printf("[thread: %d] ", hcct_thread_id);	
        for (i=0; i<indent; ++i) printf("-");
        printf("> address: %lu, call site: %hu, count: %lu\n", root->routine_id, root->call_site, root->counter);
        #endif
        
        for (ptr = root->first_child; ptr!=NULL; ptr=ptr->next_sibling)
                hcct_dump_aux(ptr, indent+1, nodes);
		
}

void hcct_dump()
{
	#if DUMP_STATS==1
	unsigned long nodes=0;
	hcct_dump_aux(hcct_get_root(), 1, &nodes);
	printf("[thread: %d] Total number of nodes: %lu\n", hcct_thread_id, nodes);	
	#endif
}
