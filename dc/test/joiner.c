/* ============================================================================
 *  joiner.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 11, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/15 09:54:42 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.3 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "joiner.h"
#include "dc.h"

// macros
#ifndef DEBUG
#define DEBUG 2
#endif

// global hash table for joiner objects
static GHashTable* hash;

// global counter of allocated joiner objects
static unsigned objcount;


// joiner object
struct joiner_s {
    joiner_t* generator; // generator splitter   
    rnode_t** in1;       // input list 1
    rnode_t** in2;       // input list 2
    rnode_t** out;       // output list
    joiner_t* next;      // pointer to next joiner
    int       cons_id;   // constraint associated with output node
    int       refcount;  // object reference count for garbage collection
};


// prototypes
static void joiner_cons(joiner_t* m);
static joiner_t* joiner_create(rnode_t** in1, rnode_t** in2, 
    rnode_t** out, size_t refcount, joiner_t* gen);
static void joiner_final(joiner_t* m);


// ---------------------------------------------------------------------
//  joiner_create
// ---------------------------------------------------------------------
static joiner_t* joiner_create(rnode_t** in1, rnode_t** in2, 
    rnode_t** out, size_t refcount, joiner_t* gen) {

    // create hash table
    if (hash == NULL) {
        #if DEBUG > 1
        printf("[joiner] creating hash table\n");
        #endif
        hash = g_hash_table_new(NULL, NULL);
        if (hash == NULL) return NULL;
    }

    // allocate joiner object
    joiner_t* m = (joiner_t*)malloc(sizeof(joiner_t));
    if (m == NULL) return NULL;

    // initialize it
    m->generator = gen;
    m->in1       = in1;
    m->in2       = in2;
    m->out       = out;
    m->next      = NULL;
    m->refcount  = refcount;

    // associate this joiner to head node of input list
    g_hash_table_insert(hash, in1, m);

    // increase global object count
    objcount++;

    #if DEBUG > 1
    printf("[joiner %p] >> new joiner created - added key %p to hash table\n", m, in1);
    #endif

    // create and run constraint
    m->cons_id = newcons((cons_t)joiner_cons, m);

    return m;
}


// ---------------------------------------------------------------------
//  joiner_final
// ---------------------------------------------------------------------
void joiner_final(joiner_t* m) {

    #if DEBUG > 0
    printf("[joiner %p] >> final execution - refcount: %d - list head: ", 
        m, m->refcount);
    if (m->refcount <= 0) printf("DEAD\n");
    else if (*m->in1 == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in1)->val);
    #endif

    // remove reference to this joiner from hash table
    if (!g_hash_table_remove(hash, m->in1)) {
        printf("[joiner %p]    internal error\n", m);
        exit(1);
    }

    #if DEBUG > 1
    printf("[joiner %p]    removed key %p from hash table\n", m, m->in1);
    #endif

    // dispose of constraint associated with this joiner
    delcons(m->cons_id);

    // decrease reference counter of next joiner, if any
    if (m->next != NULL)
        if (--m->next->refcount == 0)
            joiner_final(m->next);

    // deallocate node of output list maintained by this joiner
    // if refcount == -1 then the joiner is a generator and has no
    // associated node
    if (m->refcount == -1) *m->out = *m->in2;
    else rfree(container_of(m->out, rnode_t, next));

    // dispose of joiner object 
    free(m);

    #if DEBUG > 1
    printf("[joiner %p]    destroyed\n", m);
    #endif

    // update global object count
    if (--objcount == 0) {
        #if DEBUG > 1
        printf("[joiner] ** disposing of hash table\n");
        #endif
        g_hash_table_destroy(hash);
        hash = NULL;
    }
}


// ---------------------------------------------------------------------
//  joiner_cons
// ---------------------------------------------------------------------
static void joiner_cons(joiner_t* m) {

    // pointer to next joiner, or NULL if input list is empty
    joiner_t* next;

    #if DEBUG > 0
    printf("[joiner %p] -- cons execution for lists:\n", m);
    if (m->refcount == 0) printf("DEAD\n");
    else {
        printf("[joiner %p]       in1: ", m); rlist_inactive_print(*m->in1);
        printf("[joiner %p]       in2: ", m); rlist_inactive_print(*m->in2);
    }
    #if 0
    if (*m->in1 == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in1)->val);
    #endif
    #endif
    
    // if reference count of joiner has dropped to zero, do nothing
    // (joiner m will be removed by final handler)
    if (m->refcount == 0) {
        #if DEBUG > 1
        printf("[joiner %p]    joiner dead (refcount==0)\n", m);
        #endif
        return;
    }

    // get pointer to joiner associated with head node of input list
    if (*m->in1 == NULL) next = NULL;
    else {

        // find joiner associated with head node of input list
        next = g_hash_table_lookup(hash, &(*m->in1)->next);

        #if DEBUG > 1
        printf("[joiner %p]    next joiner lookup yielded %p\n", m, next);
        #endif

        // if no joiner is known for the node, then create new one
        if (next == NULL) {

            // create head node of output list
            rnode_t* node = (rnode_t*)rcalloc(sizeof(rnode_t), 1);
            if (node == NULL) {
                puts("[joiner]    out of memory");
                exit(1);
            }

            // create new joiner
            next = joiner_create(&(*m->in1)->next, m->in2, &node->next, 0, m->generator);
            if (next == NULL) {
                puts("[joiner]    out of memory");
                exit(1);
            }
        }
    }

    // if next joiner has changed
    if (m->next != next) {

        #if DEBUG > 1
        printf("[joiner %p]    relinking from %p to %p\n", m, m->next, next);
        #endif
        
        // increase reference counter of new next joiner, if any
        if (next != NULL) 
            if (next->refcount++ == 0) 
                schedule_final(next->cons_id, NULL);

        // decrease reference counter of old next joiner, if any
        if (m->next != NULL) {

            if (--m->next->refcount == 0)
                schedule_final(m->next->cons_id, (cons_t)joiner_final);            

            #if DEBUG > 1
            printf("[joiner %p]    decreased to %d reference counter of joiner %p\n",
                m, m->next->refcount, m->next);
            #endif
        }

        // link m to new joiner
        m->next = next;
    }

    // update output list
    if (next != NULL) {
        container_of(next->out, rnode_t, next)->val = (*m->in1)->val;
        *m->out = container_of(next->out, rnode_t, next);
    } else *m->out = *m->in2;
}


// ---------------------------------------------------------------------
// joiner_new
// ---------------------------------------------------------------------
joiner_t* joiner_new(rnode_t** in1, rnode_t** in2, rnode_t** out) {
    
    // there can be only one joiner associated with head of input list
    if (hash != NULL && g_hash_table_lookup(hash, in1) != NULL) 
        return NULL;

    joiner_t* j = joiner_create(in1, in2, out, 1, NULL);
    j->generator = j;
    return j;
}


// ---------------------------------------------------------------------
// joiner_delete
// ---------------------------------------------------------------------
void joiner_delete(joiner_t* m) {
    
    // schedule generator node for deletion
    m->refcount = -1;
    joiner_final(m);
    
    // reset output list
    *m->out = NULL;
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
