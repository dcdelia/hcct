/* ============================================================================
 *  splitter.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 1, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 15:13:06 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.9 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "splitter.h"
#include "dc.h"

// macros
#ifndef DEBUG
#define DEBUG 2
#endif

// global hash table for splitter objects
static GHashTable* hash;

// global counter of allocated splitter objects
static unsigned objcount;

// heads of output lists
typedef struct {
    rnode_t **small;       // list of items <= than the pivot
    rnode_t **large;       // list of items >= than the pivot
} out_t;

// splitter object
struct splitter_s {
    rnode_t**   in;        // input list
    rnode_t*    node;      // output node
    out_t*      out;       // output list heads and pivot
    int*        pivot;     // pivot item
    splitter_t* next;      // pointer to next splitter
    int         cons_id;   // constraint associated with output node
    int         refcount;  // object reference count for garbage collection
};


// prototypes
static void splitter_cons(splitter_t* s);
static splitter_t* splitter_create(rnode_t** in, int* pivot);
static void splitter_final(splitter_t* s);



// ---------------------------------------------------------------------
//  splitter_create
// ---------------------------------------------------------------------
static splitter_t* splitter_create(rnode_t** in, int* pivot) {

    // create hash table
    if (hash == NULL) {
        #if DEBUG > 1
        printf("[splitter] creating hash table\n");
        #endif
        hash = g_hash_table_new(NULL, NULL);
        if (hash == NULL) return NULL;
    }

    // allocate splitter object
    splitter_t* s = (splitter_t*)malloc(sizeof(splitter_t));
    if (s == NULL) return NULL;

    // initialize it
    s->in       = in;
    s->next     = NULL;
    s->refcount = 0;
    s->pivot    = pivot;
    s->out      = (out_t*)rmalloc(sizeof(out_t));
    if (s->out == NULL) { free(s); return NULL; }

    // associate this splitter to head node of input list
    g_hash_table_insert(hash, in, s);

    // increase global object count
    objcount++;

    #if DEBUG > 1
    printf("[splitter %p] >> new splitter created - added key %p to hash table\n", s, in);
    #endif

    // create and run constraint
    s->cons_id = newcons((cons_t)splitter_cons, s);

    return s;
}


// ---------------------------------------------------------------------
//  splitter_final
// ---------------------------------------------------------------------
void splitter_final(splitter_t* s) {

    #if DEBUG > 0
    printf("[splitter %p] >> final execution - refcount: %d - list head: ", 
        s, s->refcount);
    if (s->refcount <= 0) printf("DEAD\n");
    else if (*s->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*s->in)->val);
    #endif

    // remove reference to this splitter from hash table
    if (!g_hash_table_remove(hash, s->in)) {
        printf("[splitter %p]    internal error\n", s);
        exit(1);
    }

    #if DEBUG > 1
    printf("[splitter %p]    removed key %p from hash table\n", s, s->in);
    #endif

    // dispose of constraint associated with this splitter
    delcons(s->cons_id);

    // decrease reference counter of next splitter, if any
    if (s->next != NULL)
        if (--s->next->refcount == 0)
            splitter_final(s->next);

    // deallocate node of output list maintained by this splitter
    // if refcount == -1, then the splitter is a generator and has no
    // associated node
    if (s->refcount == -1) *s->out->small = *s->out->large = NULL;
    else free(s->node);

    // dispose of out list heads
    rfree(s->out);

    // dispose of splitter object 
    free(s);

    #if DEBUG > 1
    printf("[splitter %p]    destroyed\n", s);
    #endif

    // update global object count
    if (--objcount == 0) {
        #if DEBUG > 1
        printf("[splitter] ** disposing of hash table\n");
        #endif
        g_hash_table_destroy(hash);
        hash = NULL;
    }
}


// ---------------------------------------------------------------------
//  splitter_cons
// ---------------------------------------------------------------------
static void splitter_cons(splitter_t* s) {

    // pointer to next splitter, or NULL if input list is empty
    splitter_t* next;

    #if DEBUG > 0
    printf("[splitter %p] -- cons execution for list head: ", s);
    if (s->refcount == 0) printf("DEAD\n");
    else if (*s->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*s->in)->val);
    #endif
    
    // if reference count of splitter has dropped to zero, do nothing
    if (s->refcount == 0) {
        #if DEBUG > 1
        printf("[splitter %p]    splitter dead (refcount==0)\n", s);
        #endif
        return;
    }

    // get pointer to splitter associated with head node of input list
    if (*s->in == NULL) next = NULL;
    else {

        // find splitter associated with head node of input list
        next = g_hash_table_lookup(hash, &(*s->in)->next);

        #if DEBUG > 1
        printf("[splitter %p]    next splitter lookup yielded %p\n", s, next);
        #endif

        // if no splitter is known for the node, then create new one
        if (next == NULL) {

            // create new splitter
            next = splitter_create(&(*s->in)->next, s->pivot);
            if (next == NULL) {
                puts("[splitter]    out of memory");
                exit(1);
            }

            // create output node associated with the created splitter
            next->node = (rnode_t*)malloc(sizeof(rnode_t));
            if (next->node == NULL) {
                puts("[splitter]    out of memory");
                exit(1);
            }
        }
    }

    // if next splitter has changed
    if (s->next != next) {

        #if DEBUG > 1
        printf("[splitter %p]    relinking from %p to %p\n", s, s->next, next);
        #endif

        // increase reference counter of new next splitter, if any
        if (next != NULL) {
            if (next->refcount++ == 0) 
                schedule_final(next->cons_id, NULL);

            #if DEBUG > 1
            printf("[splitter %p]    increased to %d reference counter of splitter %p\n",
                s, next->refcount, next);
            #endif
        }

        // decrease reference counter of old next splitter, if any
        if (s->next != NULL) {
            if (--s->next->refcount == 0)
                schedule_final(s->next->cons_id, (cons_t)splitter_final);            

            #if DEBUG > 1
            printf("[splitter %p]    decreased to %d reference counter of splitter %p\n",
                s, s->next->refcount, s->next);
            #endif
        }

        // link s to new splitter
        s->next = next;
    }

    if (next != NULL) {

        // sync output value
        next->node->val = (*s->in)->val;

        // hook to either of the output lists
        if ( next->node->val <  *s->pivot  ||
            (next->node->val == *s->pivot && rand() % 2)) {

            #if DEBUG > 1
            printf("[splitter %p]    hooking to small output list\n", s);
            #endif
            *s->out->small   = next->node;
            next->out->small = &next->node->next;
            next->out->large = s->out->large;
        }
        else {
            #if DEBUG > 1
            printf("[splitter %p]    hooking to large output list\n", s);
            #endif
            *s->out->large   = next->node;
            next->out->small = s->out->small;
            next->out->large = &next->node->next;
        }
    } 
    else *s->out->small = *s->out->large = NULL;

    #if DEBUG > 2
    printf("[splitter %p] -- end cons\n", s);
    #endif
}


// ---------------------------------------------------------------------
// splitter_new
// ---------------------------------------------------------------------
splitter_t* splitter_new(rnode_t** in, rnode_t** small, rnode_t** large, int* pivot) {

    // there can be only one splitter associated with head of input list
    if (hash != NULL && g_hash_table_lookup(hash, in) != NULL) 
        return NULL;

    begin_at();

    // create generator splitter with special reference counter -1
    splitter_t* s = splitter_create(in, pivot);
    if (s == NULL) return NULL;

    // setup object
    s->refcount      = -1;
    s->out->small    = small;
    s->out->large    = large;

    end_at();

    return s;
}


// ---------------------------------------------------------------------
// splitter_delete
// ---------------------------------------------------------------------
void splitter_delete(splitter_t* s) {

    // schedule generator node for deletion
    s->refcount = -1;
    splitter_final(s);
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
