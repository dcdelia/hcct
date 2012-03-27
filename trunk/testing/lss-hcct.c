#include <stdio.h>
#include <stdlib.h>

#include "allocator/pool.h"
#include "lss-hcct.h"

// swap macro
#define swap_and_decr(a,b) {            \
    lss_hcct_node_t *tmp = queue[a];    \
    queue[a] = queue[b];                \
    queue[b--] = tmp;                   \
}

// monitoring macros
#define IsMonitored(x)      ((x)->additional_info==1)
#define SetMonitored(x)     ((x)->additional_info=1)
#define UnsetMonitored(x)   ((x)->additional_info=0)

// globals
__thread unsigned          stack_idx;
__thread lss_hcct_node_t  *stack[STACK_MAX_DEPTH];
__thread lss_hcct_node_t  *hcct_root;
__thread pool_t           *node_pool;
__thread void             *free_list;
__thread UINT32            min, epsilon, phi;
__thread UINT32            min_idx, num_queue_items, second_min_idx;
__thread lss_hcct_node_t **queue;
__thread int               queue_full;

// TODO
extern __thread pid_t hcct_thread_id;

lss_hcct_node_t* hcct_get_root() {
    return hcct_root;
}

#if INLINE_UPD_MIN==0
void update_min() __attribute__((no_instrument_function))
{

    #if 0 // O(n) - classic minimum search
    int i;
    min=0xFFFFFFFF;
    for (i=0; i<num_queue_items; ++i)
        if (queue[i]->counter < min) {
            min=queue[i]->counter;
            min_idx=i;
        }
    #else

    #if 1 // See paper for details
    int i=min_idx+1; // Obviously, min has changed!
    
    for (; i < num_queue_items && queue[i]->counter != min; ++i);
        
    if (i>=num_queue_items) {
        min=0xFFFFFFFF; // SENZA NON FUNZIONA: PERCHÉ????
        // IF: perche' il vecchio valore di min e' piu' piccolo
        // del minimo valore attualmente nell'array
        for (i=0; i<num_queue_items; ++i) {
            if (queue[i]->counter < min) {
                min=queue[i]->counter;
                min_idx=i;
            }
        }
    } else {
        min=queue[i]->counter;
        min_idx=i;
    }

    #else // Another version (faster? maybe slower!)

    int i=min_idx+1, j=0;
    UINT32 tmp=queue[min_idx]->counter;
    
    for (; i < num_queue_items && queue[i]->counter !=min; ++i) {
        if (queue[i]->counter < tmp) { // keeping track of minimum
            j=i;
            tmp=queue[i]->counter;
        }        
    }
    
    if (i>=num_queue_items) {
        min=queue[min_idx]->counter;
        for (i=0; i<min_idx; ++i) {
            if (queue[i]->counter < min) {
                min=queue[i]->counter;
                min_idx=i;
            }
        }
        if (tmp<min) {
            min=tmp;
            min_idx=j;
        }
    } else { 
        min=queue[i]->counter;
        min_idx=i;
    }
    #endif
    #endif
}
#endif

// to be used only on an unmonitored leaf!
static void prune(lss_hcct_node_t* node) __attribute__((no_instrument_function)); // PERCHÉ QUA FA QUESTA PORCHERIA???
static void prune(lss_hcct_node_t* node) {

    lss_hcct_node_t *p, *c;

    if (node==hcct_root) return;
    p=node->parent;

    if (p->first_child==node) {
        p->first_child=node->next_sibling;
        if (p->first_child==NULL && IsMonitored(p)==0)
            prune(p); // cold node becomes a leaf: can be pruned!        
    } 
    else for (c=p->first_child; c!=NULL; c=c->next_sibling)
            if (c->next_sibling==node) {
                c->next_sibling=node->next_sibling;
                break;
            }

    pool_free(node, free_list);
}

void hcct_enter(UINT32 routine_id, UINT16 call_site) {

    #if VERBOSE == 1
    printf("[lss_hcct] entering routine %lu\n", routine_id);
    #endif

    #if COMPARE_TO_CCT == 1 && BURSTING == 0
    rtn_enter_cct(routine_id, call_site);
    #endif

    lss_hcct_node_t *parent = stack[stack_idx];
    lss_hcct_node_t *node;

    // check if calling context is already in the tree
    for (node = parent->first_child; 
         node != NULL; 
         node = node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;
    
    // node was already in the tree
    if (node != NULL) {

        stack[++stack_idx] = node;

        if (IsMonitored(node)) {
            node->counter++;
            return;
        }
    } 

    // add new child to parent
    else {
        pool_alloc(node_pool, free_list, 
                   node, lss_hcct_node_t);
        node->routine_id    = routine_id;
        node->call_site     = call_site;
        node->first_child   = NULL;
        node->next_sibling  = parent->first_child;
        node->parent        = parent;
        parent->first_child = node;
        stack[++stack_idx]  = node;
    }

    // Mark node as monitored
    SetMonitored(node);

    // if table is not full
    if (!queue_full) {
        #if KEEP_EPS
        node->eps = 0;
        #endif
        node->counter = 1;
        queue[num_queue_items++] = node;
        queue_full = num_queue_items == epsilon;
        return;
    }

    #if INLINE_UPD_MIN == 1
 
    // version with swaps
    #if 0
    while (++min_idx < epsilon)
        if (queue[min_idx]->counter == min) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    min_idx = epsilon-1;
    swap_and_decr(second_min_idx, min_idx);
    for (i = 0; i <= min_idx; ++i) {
        if (queue[i]->counter > min) continue;
        if (queue[i]->counter < min)  {
            second_min_idx = i;
            min_idx = epsilon-1;
            min = queue[i]->counter;
        }
        swap_and_decr(i, min_idx);
    }
    end: 
    #endif

    #if UPDATE_MIN_SENTINEL == 1

    // version with second_min + sentinel
    while (queue[++min_idx]->counter > min);

    if (min_idx < epsilon) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    for (min_idx = 0, i = 0; i < epsilon; ++i) {
        if (queue[i]->counter < min) {
            second_min_idx = min_idx;
            min_idx = i;
            min = queue[i]->counter;
        }
    }
    queue[epsilon]->counter = min;
    end: 

    // version with second_min
    #else
    while (++min_idx < epsilon)
        if (queue[min_idx]->counter == min) goto end;

    UINT32 i;
    min = queue[second_min_idx]->counter;
    for (min_idx = 0, i = 0; i < epsilon; ++i) {
        if (queue[i]->counter < min) {
            second_min_idx = min_idx;
            min_idx = i;
            min = queue[i]->counter;
        }
    }
    end: 
    #endif
    
    #else
    // no explicit inlining: -O3 should do it anyway, though
    update_min();
    #endif

    #if KEEP_EPS
    node->eps=min;
    #endif

    node->counter = min + 1;

    UnsetMonitored(queue[min_idx]);

    if (queue[min_idx]->first_child==NULL)
        prune(queue[min_idx]);

    queue[min_idx]=node;
}

void hcct_exit()
{

    #if VERBOSE == 1
    printf("[lss-hcct] exiting routine %lu\n", 
        stack[stack_idx]->routine_id);
    #endif    

    stack_idx--;
}

// TODO: sistemare per i valori di ritorno
int hcct_init()
{
	// initialize parameters
	epsilon=EPSILON;
	phi=PHI;

    // initialize custom memory allocator
    node_pool = 
        pool_init(PAGE_SIZE, sizeof(lss_hcct_node_t), &free_list);    
    //if (node_pool == NULL) return -1;
    if (node_pool == NULL) {
			printf("[hcct] error while initializing allocator... Quitting!\n");
			exit(1);
	}
    
    // create dummy root node
    pool_alloc(node_pool, free_list, 
               hcct_root, lss_hcct_node_t);
    //if (hcct_root == NULL) return -1;
    if (hcct_root == NULL) {
			printf("[hcct] error while initializing hcct root node... Quitting!\n");
			exit(1);
	}
    
    hcct_root->first_child  = NULL;
    hcct_root->next_sibling = NULL;
    hcct_root->counter      = 1;
    hcct_root->routine_id   = 0;
    hcct_root->parent       = NULL;
    SetMonitored(hcct_root);

    // initialize stack
    stack[0]  = hcct_root;
    stack_idx = 0;

    // create lazy priority queue
    #if UPDATE_MIN_SENTINEL == 1
    queue = (lss_hcct_node_t**)malloc((epsilon+1)*sizeof(lss_hcct_node_t*));
    pool_alloc(node_pool, free_list, queue[epsilon], lss_hcct_node_t);
    //if (queue[epsilon] == NULL) return -1;
    if (queue[epsilon] == NULL) {
			printf("[hcct] error while initializing lazy priority queue... Quitting!\n");
			exit(1);
	}
    queue[epsilon]->counter = min = 0;
    #else
    queue = (lss_hcct_node_t**)malloc(epsilon*sizeof(lss_hcct_node_t*));
    #endif
    //if (queue == NULL) return -1;
    if (queue == NULL) {
			printf("[hcct] error while initializing lazy priority queue... Quitting!\n");
			exit(1);
	}
    
    queue[0]        = hcct_root;
    num_queue_items = 1; // goes from 0 to epsilon
    queue_full      = 0;
    min_idx         = epsilon-1;
    second_min_idx  = 0;

    return 0;
}

void __attribute__((no_instrument_function)) hcct_dump_aux(lss_hcct_node_t* root, int indent, unsigned long *nodes)
{
        if (root==NULL) return;
        
        (*nodes)++;
        lss_hcct_node_t* ptr;

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