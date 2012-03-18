/* ============================================================================
 *  halver.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 1, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 15:13:04 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.10 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "halver.h"
#include "dc.h"

// macros
#ifndef DEBUG
#define DEBUG 2
#endif

// global hash table for halver objects
static GHashTable* hash;

// global counter of allocated halver objects
static unsigned objcount;

// heads of output lists
typedef struct {
    rnode_t **out1, **out2;
} out_t;

// halver object
struct halver_s {
    rnode_t** in;       // input list
    out_t*    out;      // output list heads
    halver_t* next;     // pointer to next halver
    int       cons_id;  // constraint associated with output node
    int       refcount; // object reference count for garbage collection
    int       rand;     // random number associated with halver
};


// prototypes
static void halver_cons(halver_t* h);
static halver_t* halver_create(rnode_t** in);
static void halver_final(halver_t* h);


// ---------------------------------------------------------------------
//  halver_create
// ---------------------------------------------------------------------
static halver_t* halver_create(rnode_t** in) {

    // create hash table
    if (hash == NULL) {
        #if DEBUG > 1
        printf("[halver] creating hash table\n");
        #endif
        hash = g_hash_table_new(NULL, NULL);
        if (hash == NULL) return NULL;
    }

    // allocate halver object
    halver_t* h = (halver_t*)malloc(sizeof(halver_t));
    if (h == NULL) return NULL;

    // initialize it
    h->in       = in;
    h->next     = NULL;
    h->refcount = 0;
    h->out      = (out_t*)rmalloc(sizeof(out_t));
    h->rand     = rand();
    if (h->out == NULL) { free(h); return NULL; }

    // associate this halver to head node of input list
    g_hash_table_insert(hash, in, h);

    // increase global object count
    objcount++;

    #if DEBUG > 1
    printf("[halver %p] >> new halver created - added key %p to hash table\n", h, in);
    #endif

    // create and run constraint
    h->cons_id = newcons((cons_t)halver_cons, h);

    return h;
}


// ---------------------------------------------------------------------
//  halver_final
// ---------------------------------------------------------------------
void halver_final(halver_t* h) {

    #if DEBUG > 0
    printf("[halver %p] >> final execution - refcount: %d - list head: ", 
        h, h->refcount);
    if (h->refcount <= 0) printf("DEAD\n");
    else if (*h->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*h->in)->val);
    #endif

    // remove reference to this halver from hash table
    if (!g_hash_table_remove(hash, h->in)) {
        printf("[halver %p]    internal error\n", h);
        exit(1);
    }

    #if DEBUG > 1
    printf("[halver %p]    removed key %p from hash table\n", h, h->in);
    #endif

    // dispose of constraint associated with this halver
    delcons(h->cons_id);

    // decrease reference counter of next halver, if any
    if (h->next != NULL)
        if (--h->next->refcount == 0)
            halver_final(h->next);

    // deallocate node of output list maintained by this halver
    // if refcount == -1 then the halver is a generator and has no
    // associated node
    if (h->refcount == -1) *h->out->out1 = *h->out->out2 = NULL;
    else free(container_of(h->out->out1, rnode_t, next));

    // dispose of out list heads
    rfree(h->out);

    // dispose of halver object 
    free(h);

    #if DEBUG > 1
    printf("[halver %p]    destroyed\n", h);
    #endif

    // update global object count
    if (--objcount == 0) {
        #if DEBUG > 1
        printf("[halver] ** disposing of hash table\n");
        #endif
        g_hash_table_destroy(hash);
        hash = NULL;
    }
}


// ---------------------------------------------------------------------
//  halver_cons
// ---------------------------------------------------------------------
static void halver_cons(halver_t* h) {

    // pointer to next halver, or NULL if input list is empty
    halver_t* next;

    #if DEBUG > 0
    printf("[halver %p] -- cons execution for list head: ", h);
    if (h->refcount == 0) printf("DEAD\n");
    else if (*h->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*h->in)->val);
    #endif
    
    // if reference count of halver has dropped to zero, do nothing
    // (halver h will be removed by final handler)
    if (h->refcount == 0) {
        #if DEBUG > 1
        printf("[halver %p]    halver dead (refcount==0), error\n", h);
        #endif
        exit(1);
    }

    // get pointer to halver associated with head node of input list
    if (*h->in == NULL) next = NULL;
    else {

        // find halver associated with head node of input list
        next = g_hash_table_lookup(hash, &(*h->in)->next);

        #if DEBUG > 1
        printf("[halver %p]    next halver lookup yielded %p\n", h, next);
        #endif

        // if no halver is known for the node, then create new one
        if (next == NULL) {

            // create new halver
            next = halver_create(&(*h->in)->next);
            if (next == NULL) {
                puts("[halver]    out of memory");
                exit(1);
            }

            // create output node associated with the created halver
            rnode_t* node = (rnode_t*)malloc(sizeof(rnode_t));
            next->out->out1 = &node->next;
            if (node == NULL) {
                puts("[halver]    out of memory");
                exit(1);
            }
        }
    }

    // if next halver has changed
    if (h->next != next) {

        #if DEBUG > 1
        printf("[halver %p]    relinking from %p to %p\n", h, h->next, next);
        #endif

        // increase reference counter of new next halver, if any
        if (next != NULL) {
            if (next->refcount++ == 0) 
                schedule_final(next->cons_id, NULL);

            #if DEBUG > 1
            printf("[halver %p]    increased to %d reference counter of halver %p\n",
                h, next->refcount, next);
            #endif
        }

        // decrease reference counter of old next halver, if any
        if (h->next != NULL) {
            if (--h->next->refcount == 0)
                schedule_final(h->next->cons_id, (cons_t)halver_final);            

            #if DEBUG > 1
            printf("[halver %p]    decreased to %d reference counter of halver %p\n",
                h, h->next->refcount, h->next);
            #endif
        }

        // link h to new halver
        h->next = next;
    }

    // hook to either of the output lists
    if (next != NULL) {
        if (h->rand % 2) {
            #if DEBUG > 1
            printf("[halver %p]    hooking to output list 1\n", h);
            #endif
            *h->out->out1 = container_of(next->out->out1, rnode_t, next);
            next->out->out2 = h->out->out2;
        }
        else {
            #if DEBUG > 1
            printf("[halver %p]    hooking to output list 2\n", h);
            #endif
            *h->out->out2 = container_of(next->out->out1, rnode_t, next);
            next->out->out2 = h->out->out1;
        }
    } else *h->out->out1 = *h->out->out2 = NULL;

    // sync values in the output list with those of the input list
    if (h->refcount != -1) 
        container_of(h->out->out1, rnode_t, next)->val = 
            container_of(h->in, rnode_t, next)->val;
}


// ---------------------------------------------------------------------
// halver_new
// ---------------------------------------------------------------------
halver_t* halver_new(rnode_t** in, rnode_t** out1, rnode_t** out2) {
    
    // there can be only one halver associated with head of input list
    if (hash != NULL && g_hash_table_lookup(hash, in) != NULL) 
        return NULL;

    begin_at();

    // create generator halve with special reference counter -1
    halver_t* h = halver_create(in);
    if (h == NULL) return NULL;

    h->refcount  = -1;
    h->out->out1 = out1;
    h->out->out2 = out2;

    end_at();

    return h;
}


// ---------------------------------------------------------------------
// halver_delete
// ---------------------------------------------------------------------
void halver_delete(halver_t* h) {
    
    // schedule generator node for deletion
    h->refcount = -1;
    halver_final(h);
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
