/* ============================================================================
 *  adder.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 10, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/10 14:24:08 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.3 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "adder.h"
#include "dc.h"

// macros
#ifndef DEBUG
#define DEBUG 2
#endif

// global hash table for adder objects
static GHashTable* hash;

// global counter of allocated adder objects
static unsigned objcount;


// adder object
struct adder_s {
    rnode_t** in;       // input list
    int*      sum;      // sum of items in input list
    int       val;      // value of item associated with input list head
    adder_t*  next;     // pointer to next adder
    int       cons_id;  // constraint associated with output node
    int       refcount; // object reference count for garbage collection
};


// prototypes
static void adder_cons(adder_t* m);
static adder_t* adder_create(rnode_t** in, int* sum, size_t refcount);
static void adder_final(adder_t* m);


// ---------------------------------------------------------------------
//  adder_create
// ---------------------------------------------------------------------
static adder_t* adder_create(rnode_t** in, int* sum, size_t refcount) {

    // create hash table
    if (hash == NULL) {
        #if DEBUG > 1
        printf("[adder] creating hash table\n");
        #endif
        hash = g_hash_table_new(NULL, NULL);
        if (hash == NULL) return NULL;
    }

    // allocate adder object
    adder_t* m = (adder_t*)malloc(sizeof(adder_t));
    if (m == NULL) return NULL;

    // initialize it
    m->in       = in;
    m->sum      = sum;
    m->val      = 0;
    m->next     = NULL;
    m->refcount = refcount;

    // associate this adder to head node of input list
    g_hash_table_insert(hash, in, m);

    // increase global object count
    objcount++;

    #if DEBUG > 1
    printf("[adder %p] >> new adder created - added key %p to hash table\n", m, in);
    #endif

    // create and run constraint
    m->cons_id = newcons((cons_t)adder_cons, m);

    return m;
}


// ---------------------------------------------------------------------
//  adder_final
// ---------------------------------------------------------------------
void adder_final(adder_t* m) {

    #if DEBUG > 0
    printf("[adder %p] >> final execution - refcount: %d - list head: ", 
        m, m->refcount);
    if (m->refcount <= 0) printf("DEAD\n");
    else if (*m->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in)->val);
    #endif

    // remove reference to this adder from hash table
    if (!g_hash_table_remove(hash, m->in)) {
        printf("[adder %p]    internal error\n", m);
        exit(1);
    }

    #if DEBUG > 1
    printf("[adder %p]    removed key %p from hash table\n", m, m->in);
    #endif

    // dispose of constraint associated with this adder
    delcons(m->cons_id);

    // decrease reference counter of next adder, if any
    if (m->next != NULL) {

        if (--m->next->refcount == 0) {

            // take removed item off the sum
            *m->sum -= m->next->val;

            adder_final(m->next);
        }
    }

    // dispose of adder object 
    free(m);

    #if DEBUG > 1
    printf("[adder %p]    destroyed\n", m);
    #endif

    // update global object count
    if (--objcount == 0) {
        #if DEBUG > 1
        printf("[adder] ** disposing of hash table\n");
        #endif
        g_hash_table_destroy(hash);
        hash = NULL;
    }
}


// ---------------------------------------------------------------------
//  adder_cons
// ---------------------------------------------------------------------
static void adder_cons(adder_t* m) {

    // pointer to next adder, or NULL if input list is empty
    adder_t* next;

    #if DEBUG > 0
    printf("[adder %p] -- cons execution for list head: ", m);
    if (m->refcount == 0) printf("DEAD\n");
    else if (*m->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in)->val);
    #endif

    // if reference count of adder has dropped to zero, do nothing
    // (adder m will be removed by final handler)
    if (m->refcount == 0) {
        #if DEBUG > 1
        printf("[adder %p]    adder dead (refcount==0), error\n", m);
        #endif
        exit(1);
    }

    // get pointer to adder associated with head node of input list
    if (*m->in == NULL) next = NULL;
    else {

        // find adder associated with head node of input list
        next = g_hash_table_lookup(hash, &(*m->in)->next);

        #if DEBUG > 1
        printf("[adder %p]    next adder lookup yielded %p\n", m, next);
        #endif

        // if no adder is known for the node, then create new one
        if (next == NULL) {

            // create new adder
            next = adder_create(&(*m->in)->next, m->sum, 0);
            if (next == NULL) {
                puts("[adder]    out of memory");
                exit(1);
            }
        }
    }

    // if next adder has changed
    if (m->next != next) {

        #if DEBUG > 1
        printf("[adder %p]    relinking from %p to %p\n", m, m->next, next);
        #endif

        // increase reference counter of new next adder, if any
        if (next != NULL) {
            if (next->refcount++ == 0) {

                schedule_final(next->cons_id, NULL);

                // add new item
                *m->sum += next->val;
            }

        }

        // decrease reference counter of old next adder, if any
        if (m->next != NULL) {

            if (--m->next->refcount == 0) {
                schedule_final(m->next->cons_id, (cons_t)adder_final);

                // subtract old item
                *m->sum -= m->next->val;
            }

            #if DEBUG > 1
            printf("[adder %p]    decreased to %d reference counter of adder %p\n",
                m, m->next->refcount, m->next);
            #endif
        }

        // link m to new adder
        m->next = next;
    }

    // update sum
    if (next != NULL) {
        *m->sum += (*m->in)->val - next->val;
         next->val = (*m->in)->val;
    }
}


// ---------------------------------------------------------------------
// adder_new
// ---------------------------------------------------------------------
adder_t* adder_new(rnode_t** in, int* sum) {

    if (rm_is_reactive(sum)) {
        printf("[adder] for the time being, sum variable should not be kept in reactive memory\n");
        exit(1);
    }

    // there can be only one adder associated with head of input list
    if (hash != NULL && g_hash_table_lookup(hash, in) != NULL) 
        return NULL;

    return adder_create(in, sum, 1);
}


// ---------------------------------------------------------------------
// adder_delete
// ---------------------------------------------------------------------
void adder_delete(adder_t* m) {
    
    // schedule generator node for deletion
    m->refcount = -1;
    adder_final(m);
}


/* Copyright (C) 2010 Camil Demetrescu

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
