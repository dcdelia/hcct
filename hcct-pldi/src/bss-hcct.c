/* =====================================================================
 *  bss-hcct.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 7, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#include <stdio.h>

#include "common.h"
#include "analysis.h"
#include "burst.h"
#include "metrics.h"
#include "pool.h"


// constants
#define KEEP_EPS    0
// #define COLD_ANCESTORS   0 // TODO

// hcct node structure
typedef struct bss_hcct_node_s bss_hcct_node_t;
struct bss_hcct_node_s {
    UINT32                      routine_id;    
    struct bss_hcct_bucket_s*   bucket;
    bss_hcct_node_t*            first_child;
    bss_hcct_node_t*            next_sibling;
    UINT16                      call_site;
    bss_hcct_node_t*            parent;
    bss_hcct_node_t             *bss_prev, *bss_next;
    #if KEEP_EPS
    int                         eps;    
    #endif
};

// bucket structure
typedef struct bss_hcct_bucket_s bss_hcct_bucket_t;
struct bss_hcct_bucket_s {
	UINT32              counter;
	bss_hcct_node_t*    node;
	bss_hcct_bucket_t	*next, *prev;
};

// globals
static unsigned             stack_idx;
static bss_hcct_node_t      *stack[STACK_MAX_DEPTH];
static bss_hcct_node_t      *hcct_root;
static pool_t               *node_pool, *bucket_pool;
static void                 *node_free_list, *bucket_free_list;
static UINT32               numc, numb, epsilon, phi, tau;
static bss_hcct_bucket_t    *bfirst;


// ---------------------------------------------------------------------
//  is_monitored
// ---------------------------------------------------------------------
int is_monitored(cct_node_t* node) {
    return (((bss_hcct_node_t*)node)->bucket!=NULL);
}

// ---------------------------------------------------------------------
//  prune
// ---------------------------------------------------------------------
// to be used only on an unmonitored leaf!
static void prune(bss_hcct_node_t* node) {

    bss_hcct_node_t *p, *c;

    if (node==hcct_root) return;
    p=node->parent;

    if (p->first_child==node) {
        p->first_child=node->next_sibling;
        if (p->first_child==NULL && p->bucket==NULL)
            prune(p); // cold node becomes a leaf: can be pruned!        
    } 
    else for (c=p->first_child; c!=NULL; c=c->next_sibling)
            if (c->next_sibling==node) {
                c->next_sibling=node->next_sibling;
                break;
            }

    pool_free(node, node_free_list);
}


#if COMPARE_TO_CCT==1
// ---------------------------------------------------------------------
//  finalPrune
// ---------------------------------------------------------------------
// compute HCCT from MCCT
// (for the time being, function is not called in performance tests,
//  i.e., COMPARE_TO_CCT==0)
static void finalPrune(bss_hcct_node_t* root, UINT64 N) {

    bss_hcct_node_t* child;
    if (root == NULL) return;

    for (child = root->first_child; child != NULL; child = child->next_sibling)
        finalPrune(child,N);

    if (root->first_child == NULL  && (root->bucket != NULL ||
            root->bucket->counter < ((UINT32)(N/phi)) ))
        prune(root); // cold node becomes a leaf: can be pruned!        
}
#endif


// ---------------------------------------------------------------------
//  burst_start
// ---------------------------------------------------------------------
// called from burst.c:rtn_enter_burst at the beginning of a new burst
void burst_start() {

    // TO DO: reset current position to root node
    panic("burst_start is incomplete");

    // resync context tree: this generates a sequence of rtn_enter
    do_stack_walk(rtn_enter);
}


// ---------------------------------------------------------------------
//  rtn_enter
// ---------------------------------------------------------------------
void rtn_enter(UINT32 routine_id, UINT16 call_site) {

    #if VERBOSE == 1
    printf("[bss_hcct] entering routine %lu\n", routine_id);
    #endif

    #if COMPARE_TO_CCT == 1 && BURSTING == 0
    rtn_enter_cct(routine_id, call_site);
    #endif

    bss_hcct_node_t *parent = stack[stack_idx];
    bss_hcct_node_t *node;

    // check if calling context is already in the tree
    for (node = parent->first_child; 
         node != NULL; 
         node = node->next_sibling)
        if (node->routine_id == routine_id &&
            node->call_site == call_site) break;
    
    // node was not in the tree
    if (node == NULL) {
        pool_alloc(node_pool, node_free_list, 
                   node, bss_hcct_node_t);
        node->routine_id    = routine_id;
        node->call_site     = call_site;
        node->first_child   = NULL;
        node->next_sibling  = parent->first_child;
        node->parent        = parent;
        parent->first_child = node;
        
        node->bucket        = NULL; // otherwise SEGFAULT can occur!
    }
    
    stack[++stack_idx]  = node;
    
    // node now becomes monitored
    if (node->bucket==NULL) {
         // remove first element in first bucket
		bss_hcct_node_t* r = bfirst->node;
    
		node->bucket    = bfirst;
		#if KEEP_EPS
		node->eps       = bfirst->counter;
		#endif
		node->bss_prev  = NULL; // TODO - SERVE???

		if (numc==epsilon) { // replacement step
			node->bss_next = r->bss_next;
			if (node->bss_next!=NULL)
			    // for doubly linked list coherence
				node->bss_next->bss_prev=node;
			
			if (r->first_child!=NULL) { // cold node
			    #if KEEP_EPS
				r->eps=r->bucket->counter;
				#endif
				r->bucket=NULL;
			} else prune(r);
			
		} else { // create new counter
			numc++;
			node->bss_next=r;
			r->bss_prev=node;
		}
		bfirst->node=node;
	}

    // update counter
    bss_hcct_bucket_t *b, *bplus, *newb;
	b = node->bucket;
	UINT32 cnt = b->counter;
	cnt++;
	
	// detach node
	if (node->bss_prev != NULL) {	// not the 1st node for bucket
		node->bss_prev->bss_next = node->bss_next;	
		if (node->bss_next != NULL) {
			node->bss_next->bss_prev = node->bss_prev;
		}
	} else { // node is first node for bucket
		node->bucket->node = node->bss_next;
		if (node->bss_next != NULL) {
		    node->bss_next->bss_prev= NULL;
		}
	}
	
	node->bss_next  = NULL;
	node->bss_prev  = NULL;
	
	bplus = b->next;
	if (bplus == NULL || bplus->counter != cnt) {
        // bplus needs to be created
		numb++;
		pool_alloc(bucket_pool, bucket_free_list, 
                   newb, bss_hcct_bucket_t);
		newb->counter       = cnt;
		newb->node          = node;
		newb->prev          = node->bucket;
		newb->next          = node->bucket->next;
		newb->prev->next    = newb;
		if (newb->next != NULL)
			newb->next->prev = newb;
        newb->node->bucket  = newb;
	} else {
	    // bplus exists
		node->bss_next              = bplus->node;
		bplus->node->bss_prev       = node;
		node->bss_prev              = NULL;
		bplus->node                 = node;
		node->bucket                = bplus;
	}

	if (b->node == NULL) { // bucket b is empty
		numb--;
		if (b->prev != NULL) {
			b->prev->next = b->next;
			if (b->next != NULL) {
				b->next->prev = b->prev;
			}
		} else { // b is first bucket
			bfirst = b->next;
			if (b->next != NULL) {
				b->next->prev = NULL;
			}
		}
        pool_free(b, bucket_free_list);
	}
    
}


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
    printf("[bss_hcct] tick event\n");
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
    #if COMPARE_TO_CCT == 1 // TODO check this (see lss-hcct.c)

    UINT64  N = driver->rtn_enter_events;

    #if BURSTING == 1
    UINT64  N_B = driver->rtn_enter_burst_events;
    #else
    UINT64  N_B = N;
    #endif

    UINT32  cct_threshold = (UINT32) (N   / (UINT64)phi);

    UINT32  stream_threshold = (UINT32) (N_B   / (UINT64)phi);

    metrics_write_output(f, driver, cct_threshold, stream_threshold, 
                         (cct_node_t *) hcct_root,
                         epsilon, phi, tau, is_monitored);
    
    #else
    logfile_add_to_header(f, "%-8s ", "1/eps");
    logfile_add_to_row   (f, "%-8lu ", epsilon);
    #endif
}


/*
// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void write_output(logfile_t* f, driver_t* driver) {
    
    // TODO - PROBABLY BROKEN: WE DON'T HAVE AN UINT32 COUNTER FIELD!!!

    #if COMPARE_TO_CCT == 1

    UINT64  N = driver->rtn_enter_events;
    UINT32  bss_mcct_nodes,
            bss_hcct_nodes,
            bss_hcct_cold_nodes,
            lss_hcct_false_positives, 
            lss_hcct_hot_nodes,
            H1, H2, max, uncovered=0;
    UINT64  bss_hcct_weight_sum, theSum=0;
    float   maxError,sumError=0;
    UINT32  threshold = (UINT32) (N / (UINT64)phi);

    // log number of CCT nodes
    write_output_cct(f, driver);

    // log MCCT nodes 
    bss_mcct_nodes = nodes((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-10s ", "|MCCT| ");
    logfile_add_to_row   (f, "%-10lu ", bss_mcct_nodes);    

    // build HCCT and log total and cold nodes
    finalPrune(hcct_root, N);  

    bss_hcct_nodes = nodes((cct_node_t *)hcct_root);
    
    bss_hcct_cold_nodes = 
        coldNodes((cct_node_t *)hcct_root, is_monitored);
    
    logfile_add_to_header(f, "%-10s ", "|HCCT| ");
    logfile_add_to_row   (f, "%-10lu ", bss_hcct_nodes);    

    logfile_add_to_header(f, "%-8s ", "(cold)");
    logfile_add_to_row   (f, "%-8lu ", lss_hcct_cold_nodes);    

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
    bss_hcct_weight_sum = sum((cct_node_t *)hcct_root);

    logfile_add_to_header(f, "%-12s ", "Deg overlap");
    logfile_add_to_row   (f, "%-12.2f ", ((double)bss_hcct_weight_sum / 
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
//  init
// ---------------------------------------------------------------------
int init(int argc, char** argv) {

    // get common parameters
    get_epsilon_phi_tau(argc, argv, &epsilon, &phi, &tau);

    // initialize custom memory allocator(s)
    node_pool = pool_init(PAGE_SIZE, sizeof(bss_hcct_node_t),
                            &node_free_list);
    if (node_pool == NULL) return -1;
    
    bucket_pool = pool_init(PAGE_SIZE, sizeof(bss_hcct_bucket_t),
                            &bucket_free_list);
    if (bucket_pool == NULL) return -1;
    
    // create first bucket
    pool_alloc(bucket_pool, bucket_free_list,
                    bfirst, bss_hcct_bucket_t);
	bfirst->counter = 0;
	bfirst->next    = NULL;
	bfirst->prev    = NULL;
	numc=1, numb=1;
    
    // create dummy root node
    pool_alloc(node_pool, node_free_list, 
                    hcct_root, bss_hcct_node_t);
    if (hcct_root == NULL) return -1;

    hcct_root->first_child  = NULL;
    hcct_root->next_sibling = NULL;
    hcct_root->routine_id   = 0;
    hcct_root->parent       = NULL;

    // insert dummy root node into bfirst
    hcct_root->bss_prev     = NULL;
    hcct_root->bss_next     = NULL;
    hcct_root->bucket       = bfirst;
    bfirst->node            = hcct_root;

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
