/* =====================================================================
 *  cct.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/20 07:22:13 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.15 $
*/


#include <stdio.h>

#include "analysis.h"
#include "burst.h"
#include "cct.h"
#include "metrics.h"
#include "pool.h"


#if COMPARE_TO_CCT == 0
#define __target(x) x
#else 
#define __target(x) x##_cct
#endif


// globals
static int         cct_stack_idx;
static cct_node_t *cct_stack[STACK_MAX_DEPTH];
static cct_node_t *cct_root;
static pool_t     *cct_pool;
static void       *cct_free_list;
static UINT32      cct_nodes;


// ---------------------------------------------------------------------
//  cct_get_root
// ---------------------------------------------------------------------
cct_node_t* cct_get_root(){
    return cct_root;
}

#if COMPARE_TO_CCT==0 && ADAPTIVE==1
// ---------------------------------------------------------------------
//  ctxt_lookup
// ---------------------------------------------------------------------
int ctxt_lookup(UINT32 routine_id, UINT16 call_site){
    
    cct_node_t *parent = cct_stack[cct_stack_idx];
    cct_node_t *node;

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

#if COMPARE_TO_CCT==0
// ---------------------------------------------------------------------
//  burst_start
// ---------------------------------------------------------------------
// called from burst.c:rtn_enter_burst at the beginning of a new burst
void __target(burst_start)() {

    // reset current position to dummy root node
    cct_stack_idx = 0;
    
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
#endif


// ---------------------------------------------------------------------
//  init
// ---------------------------------------------------------------------
int __target(init)(int argc, char** argv) {

    cct_stack_idx   = 0;
    cct_nodes       = 1;

    // initialize custom memory allocator
    cct_pool = pool_init(PAGE_SIZE, sizeof(cct_node_t), &cct_free_list);
    if (cct_pool == NULL) return -1;

    // dummy root node
    pool_alloc(cct_pool, cct_free_list, cct_stack[0], cct_node_t);
    if (cct_stack[0] == NULL) return -1;

    cct_stack[0]->first_child = NULL;
    cct_stack[0]->next_sibling = NULL;
    cct_root = cct_stack[0];

    return 0;
}


// ---------------------------------------------------------------------
//  cleanup
// ---------------------------------------------------------------------
void __target(cleanup)() {
    // CD101109: TO BE COMPLETED
}


// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void __target(write_output)(logfile_t* f, driver_t* driver) {
    
    #if COMPUTE_HOT_PER_PHI == 1
    
    dump_hot_per_phi(cct_root, f, driver);
    
    #else
    
    #if COMPUTE_MAX_PHI == 1

    dump_max_phi(cct_root, f, driver);
    
    #else
    
    dump_instance_data(cct_root, f, driver);

    #endif
    
    #endif
}


// ---------------------------------------------------------------------
//  rtn_enter
// ---------------------------------------------------------------------
void __target(rtn_enter)(UINT32 routine_id, UINT16 call_site) {

    #if VERBOSE == 1
    printf("[cct] entering routine %lu\n", routine_id);
    #endif

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
    pool_alloc(cct_pool, cct_free_list, node, cct_node_t);
    cct_stack[cct_stack_idx] = node;
    node->routine_id = routine_id;
    node->first_child = NULL;
    node->counter = 1;
    node->call_site =  call_site;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}

#if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
// ---------------------------------------------------------------------
//  rtn_enter_wc
// ---------------------------------------------------------------------
void __target(rtn_enter_wc)(UINT32 routine_id, UINT16 call_site,
                            UINT16 increment) {

    #if VERBOSE == 1
    printf("[cct] entering routine %lu\n", routine_id);
    #endif

    cct_node_t *parent=cct_stack[cct_stack_idx++];
    cct_node_t *node;
    for (node=parent->first_child; node!=NULL; node=node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;

    if (node!=NULL) {
        cct_stack[cct_stack_idx]=node;
        node->counter+=increment;
        return;
    }

    ++cct_nodes;
    pool_alloc(cct_pool, cct_free_list, node, cct_node_t);
    cct_stack[cct_stack_idx] = node;
    node->routine_id = routine_id;
    node->first_child = NULL;
    node->counter = increment;
    node->call_site =  call_site;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
}
#endif


// ---------------------------------------------------------------------
//  rtn_exit
// ---------------------------------------------------------------------
void __target(rtn_exit)() {

    #if VERBOSE == 1
    printf("[cct] exiting routine %lu\n", 
        cct_stack[cct_stack_idx]->routine_id);
    #endif

    cct_stack_idx--;
}


// ---------------------------------------------------------------------
//  tick
// ---------------------------------------------------------------------
void __target(tick)() {
    #if VERBOSE == 1
    printf("[cct] tick event\n");
    #endif
}


// ---------------------------------------------------------------------
//  cctPrune
// ---------------------------------------------------------------------
// compute exact HCCT from CCT
void cctPrune(cct_node_t* root, UINT32 threshold) {

    cct_node_t* child, *c;
    if (root == NULL) return;

    for (child = root->first_child; child != NULL; child = child->next_sibling) {
        cctPrune(child,threshold);
        if (child->first_child == NULL && child->counter < threshold) {
            if (root->first_child == child)
                root->first_child = child->next_sibling;
            else for (c=root->first_child; c!=NULL; c=c->next_sibling)
                    if (c->next_sibling==child) {
                        c->next_sibling = child->next_sibling;
                        break;
                    }
            pool_free(child, cct_free_list);
        }
    }
}


/* Copyright (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
