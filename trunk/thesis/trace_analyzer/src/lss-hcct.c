/* =====================================================================
 *  lss-hcct.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "analysis.h"
#include "burst.h"
#include "metrics.h"
#include "pool.h"
#include "cct.h"


// swap macro
#define swap_and_decr(a,b) {            \
    lss_hcct_node_t *tmp = queue[a];    \
    queue[a] = queue[b];                \
    queue[b--] = tmp;                   \
}

// constants
#define INLINE_UPD_MIN          1
#define UPDATE_MIN_SENTINEL     1

// eps
#if LSS_ADJUST_EPS == 1
#define KEEP_EPS                1
#else
#define KEEP_EPS                0
#endif


// monitoring macros
#define IsMonitored(x)      ((x)->additional_info==1)
#define SetMonitored(x)     ((x)->additional_info=1)
#define UnsetMonitored(x)   ((x)->additional_info=0)

// hcct node structure
typedef struct lss_hcct_node_s lss_hcct_node_t;
struct lss_hcct_node_s {
    UINT32           routine_id;    
    UINT32           counter;    
    lss_hcct_node_t* first_child;
    lss_hcct_node_t* next_sibling;
    UINT16           call_site;
    UINT16           additional_info;
    lss_hcct_node_t* parent;
    #if KEEP_EPS
    int              eps;    
    #endif
};

// globals
static unsigned          stack_idx;
static lss_hcct_node_t  *stack[STACK_MAX_DEPTH];
static lss_hcct_node_t  *hcct_root;
static pool_t           *node_pool;
static void             *free_list;
static UINT32            min, epsilon, phi, tau;
static UINT32            min_idx, num_queue_items, second_min_idx;
static lss_hcct_node_t **queue;
static int               queue_full;


//~ // Workaround for a conflict on cct_get_root() method (sse also cct.h)
//~ #if COMPARE_TO_CCT==0
// ---------------------------------------------------------------------
//  cct_get_root
// ---------------------------------------------------------------------
//~ cct_node_t* cct_get_root(){
    //~ return (cct_node_t*)hcct_root;
//~ }
//~ #else
//~ // ---------------------------------------------------------------------
//~ //  hcct_get_root
//~ // ---------------------------------------------------------------------
//~ cct_node_t* hcct_get_root(){
    //~ return (cct_node_t*)hcct_root;
//~ }
//~ #endif

#if ADAPTIVE==1
// ---------------------------------------------------------------------
//  ctxt_lookup
// ---------------------------------------------------------------------
int ctxt_lookup(UINT32 routine_id, UINT16 call_site){
    
    lss_hcct_node_t *parent = stack[stack_idx];
    lss_hcct_node_t *node;

    // check if calling context is already in the tree
    for (node = parent->first_child; 
         node != NULL; 
         node = node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;
    
    // returns 1 if node was already in the tree
    return (node != NULL);   
}
#endif


// ---------------------------------------------------------------------
//  is_monitored
// ---------------------------------------------------------------------
int is_monitored(cct_node_t* node) {
    return IsMonitored((lss_hcct_node_t *)node);
}


// ---------------------------------------------------------------------
//  update_min
// ---------------------------------------------------------------------
void update_min() {

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


// ---------------------------------------------------------------------
//  prune
// ---------------------------------------------------------------------
// to be used only on an unmonitored leaf!
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


#if COMPARE_TO_CCT==1
// ---------------------------------------------------------------------
//  finalPrune
// ---------------------------------------------------------------------
// compute HCCT from MCCT
// (for the time being, function is not called in performance tests,
//  i.e., COMPARE_TO_CCT==0)
void finalPrune(cct_node_t* root, UINT64 N) {

    lss_hcct_node_t *child;
    if (root == NULL) return;

    for (child = ((lss_hcct_node_t *)root)->first_child; 
         child != NULL; child = child->next_sibling)
            finalPrune((cct_node_t *)child,N);

    // cold node becomes a leaf: can be pruned!       
    if (root->first_child == NULL  && 
        (IsMonitored((lss_hcct_node_t *)root) == 0 || 
         root->counter < ((UINT32)(N/phi)) ))
            prune((lss_hcct_node_t *)root); 
}
#endif


// ---------------------------------------------------------------------
//  burst_start
// ---------------------------------------------------------------------
// called from burst.c:rtn_enter_burst at the beginning of a new burst
void burst_start() {

    // reset current position to dummy root node
    stack_idx = 0;

    // resync context tree
    #if ADAPTIVE==1
    
    #if WEIGHT_COMPENSATION==1
    do_stack_walk(rtn_enter_wc, ctxt_lookup);
    #else
    do_stack_walk(rtn_enter, ctxt_lookup);
    #endif
    
    #else
    do_stack_walk(rtn_enter);
    #endif
}


// ---------------------------------------------------------------------
//  rtn_enter
// ---------------------------------------------------------------------
void rtn_enter(UINT32 routine_id, UINT16 call_site) {

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

#if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
// ---------------------------------------------------------------------
//  rtn_enter_wc
// ---------------------------------------------------------------------
void rtn_enter_wc(UINT32 routine_id, UINT16 call_site,
                    UINT16 increment) {

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
            node->counter+=increment;
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
        node->counter = increment;
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

    node->counter = min + increment;

    UnsetMonitored(queue[min_idx]);

    if (queue[min_idx]->first_child==NULL)
        prune(queue[min_idx]);

    queue[min_idx]=node;
}
#endif


// ---------------------------------------------------------------------
//  rtn_exit
// ---------------------------------------------------------------------
void rtn_exit() {

    #if VERBOSE == 1
    printf("[lss-hcct] exiting routine %lu\n", 
        stack[stack_idx]->routine_id);
    #endif    
    
    #if COMPARE_TO_CCT == 1 && BURSTING == 0
    rtn_exit_cct();
    #endif

    stack_idx--;
}


// ---------------------------------------------------------------------
//  tick
// ---------------------------------------------------------------------
void tick() {
    #if VERBOSE == 1
    printf("[lss_hcct] tick event\n");
    #endif
}


// ---------------------------------------------------------------------
//  cleanup
// ---------------------------------------------------------------------
void cleanup() {

    // CD101109: TO BE COMPLETED

    #if COMPARE_TO_CCT==1
    cleanup_cct();
    #endif
}


// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void write_output(logfile_t* f, driver_t* driver) {
    #if COMPARE_TO_CCT == 1

    UINT64  N = driver->rtn_enter_events;

    UINT32  threshold = (UINT32) (N   / (UINT64)phi);

    metrics_write_output(f, driver, threshold, threshold, 
                         (cct_node_t *) hcct_root,
                         epsilon, phi, tau,
                         is_monitored);
    #else
    logfile_add_to_header(f, "%-8s ", "1/eps");
    logfile_add_to_row   (f, "%-8lu ", epsilon);
    #endif
}


// ---------------------------------------------------------------------
//  init
// ---------------------------------------------------------------------
int init(int argc, char** argv) {

    // get common parameters
    get_epsilon_phi_tau(argc, argv, &epsilon, &phi, &tau);

    // initialize custom memory allocator
    node_pool = 
        pool_init(PAGE_SIZE, sizeof(lss_hcct_node_t), &free_list);
    if (node_pool == NULL) return -1;
    
    // create dummy root node
    pool_alloc(node_pool, free_list, 
               hcct_root, lss_hcct_node_t);
    if (hcct_root == NULL) return -1;

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
    if (queue[epsilon] == NULL) return -1;
    queue[epsilon]->counter = min = 0;
    #else
    queue = (lss_hcct_node_t**)malloc(epsilon*sizeof(lss_hcct_node_t*));
    #endif
    if (queue == NULL) return -1;
    
    queue[0]        = hcct_root;
    num_queue_items = 1; // goes from 0 to epsilon
    queue_full      = 0;
    min_idx         = epsilon-1;
    second_min_idx  = 0;

    #if COMPARE_TO_CCT==1
    init_cct(argc, argv);
    #endif

    return 0;
}


/* Copyright (C) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
*/
