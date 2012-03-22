#include <stdio.h>
#include <stdlib.h>
#include "cct.h"
//#include <asm/unistd.h> // per la costante di chiamata __NR_gettid

#define STACK_MAX_DEPTH 1024

// globals
__thread int         cct_stack_idx;
__thread cct_node_t *cct_stack[STACK_MAX_DEPTH];
__thread cct_node_t *cct_root;
__thread UINT32      cct_nodes;

cct_node_t* cct_get_root() {
    return cct_root;
}

int cct_init() {

    cct_stack_idx   = 0;
    cct_nodes       = 1;

    cct_stack[0]=(cct_node_t*)malloc(sizeof(cct_node_t));

    cct_stack[0]->first_child = NULL;
    cct_stack[0]->next_sibling = NULL;
    cct_root = cct_stack[0];

    return 0;
}

void cct_enter(UINT32 routine_id, UINT16 call_site) {

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
    node=(cct_node_t*)malloc(sizeof(cct_node_t));
    cct_stack[cct_stack_idx] = node;
    node->routine_id = routine_id;
    node->first_child = NULL;
    node->counter = 1;
    node->call_site =  call_site;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}
void cct_exit() {

    cct_stack_idx--;
}

void cct_dump(cct_node_t* root, int indent) {
        if (root==NULL) return;
        
        int i;
        cct_node_t* ptr;

        for (i=0; i<indent; ++i)
                printf("-");
        printf("> thread: %lu, address: %lu, call site: %hu, count: %lu\n", (unsigned long)pthread_self(), root->routine_id, root->call_site, root->counter);
        
        for (ptr = root->first_child; ptr!=NULL; ptr=ptr->next_sibling)
                cct_dump(ptr, indent+1);
}