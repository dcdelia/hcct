/* =====================================================================
 *  lc-hcct.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 7, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/20 07:22:14 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.18 $
*/

#include <stdio.h>

#include "common.h"
#include "analysis.h"
#include "burst.h"
#include "metrics.h"
#include "pool.h"


// constants
#define KEEP_EPS         1
#define INLINE_UPD_MIN   0 // TODO serve???


// bit stealing stuff
#define MakeParent(p,bit)   ((long)(p) | (bit))
#define GetParent(x)        ((lc_hcct_node_t*) ((x->parent) & (~3)))
#define UnsetMonitored(x)   (MakeParent(GetParent(x), 0))
#define IsMonitored(x)      ((x)->parent & 1)

// hcct node structure
typedef struct lc_hcct_node_s lc_hcct_node_t;
struct lc_hcct_node_s {
    UINT32          routine_id;    
    UINT32          counter;    
    lc_hcct_node_t* first_child;
    lc_hcct_node_t* next_sibling;
    UINT16          call_site;
    UINT32          parent; // bit stealing field
    lc_hcct_node_t* lc_next; // LC on a linked list
    int             delta;
};

// globals
static unsigned         stack_idx;
static lc_hcct_node_t   *stack[STACK_MAX_DEPTH];
static lc_hcct_node_t   *hcct_root;
static pool_t           *node_pool;
static void             *free_list;
static UINT32           epsilon, phi, tau;

// LC internals
static UINT32           bucket, index; // N == bucket*epsilon + index :)
static lc_hcct_node_t   *lc_root;


//~ // Workaround for a conflict on cct_get_root() method (sse also cct.h)
//~ #if COMPARE_TO_CCT==0
//~ // ---------------------------------------------------------------------
//~ //  cct_get_root
//~ // ---------------------------------------------------------------------
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
    
    lc_hcct_node_t *parent = stack[stack_idx];
    lc_hcct_node_t *node;

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
    return IsMonitored((lc_hcct_node_t *)node);
}


// ---------------------------------------------------------------------
//  prune
// ---------------------------------------------------------------------
// to be used only on an unmonitored leaf!
static void prune(lc_hcct_node_t* node) {

    lc_hcct_node_t *p, *c;

    if (node==hcct_root) return;
    p=GetParent(node);

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


// ---------------------------------------------------------------------
//  lc_prune
// ---------------------------------------------------------------------
// to be used only when window boundary (i.e. index=epsilon) is reached
static void lc_prune(lc_hcct_node_t* current_node) {
    lc_hcct_node_t *c=lc_root, *tmp;
    
    if (c->lc_next==current_node) // maybe not necessary at all
        c=c->lc_next;
    
    while (c->lc_next!=NULL)
        if (c->lc_next->counter + c->lc_next->delta <= bucket + 1) {
            tmp         = c->lc_next;
            c->lc_next  = c->lc_next->lc_next;
            tmp->parent = UnsetMonitored(tmp);
            if (tmp->first_child==NULL)
                prune(tmp);
        } else c=c->lc_next;
        
    index=0;
    bucket++;
}


#if COMPARE_TO_CCT==1
// ---------------------------------------------------------------------
//  finalPrune
// ---------------------------------------------------------------------
// compute HCCT from MCCT
// (for the time being, function is not called in performance tests,
//  i.e., COMPARE_TO_CCT==0)
void finalPrune(cct_node_t* root, UINT64 N) {

    lc_hcct_node_t* child;
    if (root == NULL) return;

    for (child = ((lc_hcct_node_t *)root)->first_child; 
         child != NULL; child = child->next_sibling)
            finalPrune((cct_node_t *)child,N);

    // Manku-Motwani's deletion rule: count < (phi-eps)*N
    // However, delta keeps track of the lowest proper underestimation 
    // This implementation exploits deltas: count+delta < phi*N
    // => reduces % of false positives by a factor ~ 2 in our tests
    
    // cold node becomes a leaf: can be pruned!        
    if (root->first_child == NULL && 
        (IsMonitored((lc_hcct_node_t *)root) == 0 ||
        root->counter + ((lc_hcct_node_t *)root)->delta < ((UINT32)(N/phi)) ))
            prune((lc_hcct_node_t *)root);     
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
    printf("[lc_hcct] entering routine %lu\n", routine_id);
    #endif

    #if COMPARE_TO_CCT==1 && BURSTING == 0
    rtn_enter_cct(routine_id, call_site);
    #endif

    lc_hcct_node_t *parent = stack[stack_idx];
    lc_hcct_node_t *node;
    
    // Keeping track of stream length % epsilon
    ++index;
    
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
        } else {
            // adding this node at the beginning of the linked list
            node->lc_next=lc_root->lc_next;
            lc_root->lc_next=node;
            // node was a cold ancestor, so delta has not to be updated
            // => this should make # of false positives smaller!
            node->parent = MakeParent(parent, 1);
            node->counter++;
        }
    
    } else {

        // add new child to parent
        pool_alloc(node_pool, free_list, node, lc_hcct_node_t);
        node->routine_id    = routine_id;
        node->call_site     = call_site;
        node->first_child   = NULL;
        node->next_sibling  = parent->first_child;
        parent->first_child = node;
        stack[++stack_idx]  = node;
    
        // set parent node and counter(s)
        node->parent    = MakeParent(parent, 1);
        node->counter   = 1;
        node->delta     = bucket;
    
        // adding this node at the beginning of the linked list
        node->lc_next=lc_root->lc_next;
        lc_root->lc_next=node;
    }
    
    if (index==epsilon)
        lc_prune(node);
}


#if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
// ---------------------------------------------------------------------
//  rtn_enter_wc
// ---------------------------------------------------------------------
void rtn_enter_wc(UINT32 routine_id, UINT16 call_site,
                    UINT16 increment) {
                        
    #if VERBOSE == 1
    printf("[lc_hcct] entering routine %lu\n", routine_id);
    #endif

    #if COMPARE_TO_CCT==1 && BURSTING == 0
    rtn_enter_cct(routine_id, call_site);
    #endif

    lc_hcct_node_t *parent = stack[stack_idx];
    lc_hcct_node_t *node;
    
    // Keeping track of stream length % epsilon
    index+=increment;
    int res=index-epsilon;
    
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
        } else {
            // adding this node at the beginning of the linked list
            node->lc_next=lc_root->lc_next;
            lc_root->lc_next=node;
            // node was a cold ancestor, so delta has not to be updated
            // => this should make # of false positives smaller!
            node->parent = MakeParent(parent, 1);
            node->counter+=increment;
        }
    
    } else {

        // add new child to parent
        pool_alloc(node_pool, free_list, node, lc_hcct_node_t);
        node->routine_id    = routine_id;
        node->call_site     = call_site;
        node->first_child   = NULL;
        node->next_sibling  = parent->first_child;
        parent->first_child = node;
        stack[++stack_idx]  = node;
    
        // set parent node and counter(s)
        node->parent    = MakeParent(parent, 1);
        node->counter   = increment;
        node->delta     = bucket;
    
        // adding this node at the beginning of the linked list
        node->lc_next=lc_root->lc_next;
        lc_root->lc_next=node;
    }
    
    if (index>=epsilon)
        lc_prune(node);
    
    if (res>0) index+=res;
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
    
    #if COMPARE_TO_CCT==1 && BURSTING == 0
    rtn_exit_cct();
    #endif

    stack_idx--;
}


// ---------------------------------------------------------------------
//  tick
// ---------------------------------------------------------------------
void tick() {
    #if VERBOSE == 1
    printf("[lc_hcct] tick event\n");
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

/*
// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void write_output(logfile_t* f, driver_t* driver) {

    #if COMPARE_TO_CCT == 1

    UINT64  N = driver->rtn_enter_events;
    UINT32  lss_mcct_nodes,
            lc_hcct_nodes,
            lc_hcct_cold_nodes,
            lss_hcct_false_positives, 
            lss_hcct_hot_nodes,
            H1, H2, max, uncovered=0;
    UINT64  lc_hcct_weight_sum, theSum=0;
    float   maxError,sumError=0;
    UINT32  threshold = (UINT32) (N / (UINT64)phi);

    // log number of CCT nodes
    write_output_cct(f, driver);

    // log MCCT nodes 
    lss_mcct_nodes = nodes((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-10s ", "|MCCT| ");
    logfile_add_to_row   (f, "%-10lu ", lss_mcct_nodes);    

    // build HCCT and log total and cold nodes
    finalPrune(hcct_root, N);  

    lc_hcct_nodes = nodes((cct_node_t *)hcct_root);
    
    lc_hcct_cold_nodes = 
        coldNodes((cct_node_t *)hcct_root, is_monitored);
    
    logfile_add_to_header(f, "%-10s ", "|HCCT| ");
    logfile_add_to_row   (f, "%-10lu ", lc_hcct_nodes);    

    logfile_add_to_header(f, "%-9s ", "(cold)");
    logfile_add_to_row   (f, "%-9lu ", lc_hcct_cold_nodes);    

    // change counters in HCCT to their exact values before computing 
    // remaining metrics
    exactCounters((cct_node_t *)hcct_root, cct_get_root(),
                  &maxError, &sumError, threshold, is_monitored);

    lss_hcct_hot_nodes = 
        hotNodes((cct_node_t *)hcct_root, threshold, is_monitored);

    lss_hcct_false_positives = 
        falsePositives((cct_node_t *)hcct_root, threshold, is_monitored);

    logfile_add_to_header(f, "%-8s ", "(hot)");
    logfile_add_to_row   (f, "%-8lu ", lss_hcct_hot_nodes);    

    logfile_add_to_header(f, "%-8s ", "(false)");
    logfile_add_to_row   (f, "%-8lu ", lss_hcct_false_positives);    

    // compute sum of counters in HCCT
    lc_hcct_weight_sum = sum((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-12s ", "Deg overlap");
    logfile_add_to_row   (f, "%-12.2f ", ((double)lc_hcct_weight_sum / 
                                         (double)N) * 100);    

    uncoveredFrequency((cct_node_t *)hcct_root, cct_get_root(),
                        &theSum, &max, &uncovered);

    // compute hottest counter in HCCT
    H1 = hottest((cct_node_t *)hcct_root);

    // compute hottest counter in CCT
    H2 = hottest(cct_get_root());
 
    // log avg counter value of nodes not included in HCCT
    logfile_add_to_header(f, "%-13s ", "Avg uncovered");
    logfile_add_to_row   (f, "%-13lu ",
                            (UINT32)(theSum/(UINT64)uncovered));    

    logfile_add_to_header(f, "%-6s ", "(%)");
    logfile_add_to_row   (f, "%-6.3f ",
                            ((double)((double)theSum/
                            (double)uncovered))*100/(double)H1);    

    // log max counter value of nodes not included in HCCT
    logfile_add_to_header(f, "%-13s ", "Max uncovered");
    logfile_add_to_row   (f, "%-13lu ",max);    

    logfile_add_to_header(f, "%-6s ", "(%)");
    logfile_add_to_row   (f, "%-6.3f ",(double)max*100/(double)H1);    

    logfile_add_to_header(f, "%-9s ", "phi*N");
    logfile_add_to_row   (f, "%-9lu ", threshold);    

    logfile_add_to_header(f, "%-6s ", "1/tau");
    logfile_add_to_row   (f, "%-6lu ", tau);    

    logfile_add_to_header(f, "%-10s ", "Max hcct");
    logfile_add_to_row   (f, "%-10lu ", H1);    

    logfile_add_to_header(f, "%-10s ", "Max cct");
    logfile_add_to_row   (f, "%-10lu ", H2);    

    logfile_add_to_header(f, "%-9s ", "Hot cov.");
    logfile_add_to_row   (f, "%-9.2f ", 
            (((double)larger_than_hottest((cct_node_t *)hcct_root,
            tau,H1)) /
            ((double)larger_than_hottest(cct_get_root(),tau,H2)))*100);

    logfile_add_to_header(f, "%-8s ", "min tau");
    logfile_add_to_row   (f, "%-8.4f ", (float)threshold/H1);    

    logfile_add_to_header(f, "%-10s ", "max error");
    logfile_add_to_row   (f, "%-10.5f ", maxError);    

    logfile_add_to_header(f, "%-10s ", "avg error");
    logfile_add_to_row   (f, "%-10.5f ", sumError/lss_hcct_hot_nodes);    

    logfile_add_to_header(f, "%-7s ", "1/phi");
    logfile_add_to_row   (f, "%-7lu ", phi);    

    logfile_add_to_header(f, "%-8s ", "1/eps");
    logfile_add_to_row   (f, "%-8lu ", epsilon);

    #endif
}
*/


// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void write_output(logfile_t* f, driver_t* driver) {
    #if COMPARE_TO_CCT == 1

    UINT64  N = driver->rtn_enter_events;

    UINT32  cct_threshold = (UINT32) (N   / (UINT64)phi);

    UINT32  stream_threshold =  (UINT32) (N   / (UINT64)phi) - 
                                (UINT32) (N   / (UINT64)epsilon);

    metrics_write_output(f, driver, cct_threshold, 
                         stream_threshold, (cct_node_t *) hcct_root,
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
        pool_init(PAGE_SIZE, sizeof(lc_hcct_node_t), &free_list);
    if (node_pool == NULL) return -1;
    
    // create dummy root node
    pool_alloc(node_pool, free_list, hcct_root, lc_hcct_node_t);
    if (hcct_root == NULL) return -1;
    
    // using a dummy head node in the linked list of monitored elements
    pool_alloc(node_pool, free_list, lc_root, lc_hcct_node_t);
    if (lc_root == NULL) return -1;

    hcct_root->first_child  = NULL;
    hcct_root->next_sibling = NULL;
    hcct_root->counter      = 1;
    hcct_root->routine_id   = 0;
    hcct_root->parent       = MakeParent(NULL, 1);
    hcct_root->lc_next      = NULL;
    hcct_root->delta        = 0;
    lc_root->lc_next        = hcct_root;
    bucket                  = 0;
    index                   = 1;

    // initialize stack
    stack[0]  = hcct_root;
    stack_idx = 0;
    
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
