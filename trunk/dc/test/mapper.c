/* ============================================================================
 *  mapper.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 1, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 15:13:05 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.7 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "mapper.h"
#include "dc.h"

// macros
#ifndef DEBUG
#define DEBUG 2
#endif

// global hash table for mapper objects
static GHashTable* hash;

// global counter of allocated mapper objects
static unsigned objcount;


// mapper object
struct mapper_s {
    rnode_t** in;       // input list
    rnode_t** out;      // output list
    map_t     map;      // map function
    mapper_t* next;     // pointer to next mapper
    int       cons_id;  // constraint associated with output node
    int       refcount; // object reference count for garbage collection
};


// prototypes
static void mapper_cons(mapper_t* m);
static mapper_t* mapper_create(rnode_t** in, rnode_t** out, map_t map, size_t refcount);
static void mapper_final(mapper_t* m);


// ---------------------------------------------------------------------
//  mapper_create
// ---------------------------------------------------------------------
static mapper_t* mapper_create(rnode_t** in, rnode_t** out, map_t map, size_t refcount) {

    // create hash table
    if (hash == NULL) {
        #if DEBUG > 1
        printf("[mapper] creating hash table\n");
        #endif
        hash = g_hash_table_new(NULL, NULL);
        if (hash == NULL) return NULL;
    }

    // allocate mapper object
    mapper_t* m = (mapper_t*)malloc(sizeof(mapper_t));
    if (m == NULL) return NULL;

    // initialize it
    m->in       = in;
    m->out      = out;
    m->map      = map;
    m->next     = NULL;
    m->refcount = refcount;

    // associate this mapper to head node of input list
    g_hash_table_insert(hash, in, m);

    // increase global object count
    objcount++;

    #if DEBUG > 1
    printf("[mapper %p] >> new mapper created - added key %p to hash table\n", m, in);
    #endif

    // create and run constraint
    m->cons_id = newcons((cons_t)mapper_cons, m);

    return m;
}


// ---------------------------------------------------------------------
//  mapper_final
// ---------------------------------------------------------------------
void mapper_final(mapper_t* m) {

    #if DEBUG > 0
    printf("[mapper %p] >> final execution - refcount: %d - list head: ", 
        m, m->refcount);
    if (m->refcount <= 0) printf("DEAD\n");
    else if (*m->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in)->val);
    #endif

    // remove reference to this mapper from hash table
    if (!g_hash_table_remove(hash, m->in)) {
        printf("[mapper %p]    internal error\n", m);
        exit(1);
    }

    #if DEBUG > 1
    printf("[mapper %p]    removed key %p from hash table\n", m, m->in);
    #endif

    // dispose of constraint associated with this mapper
    delcons(m->cons_id);

    // decrease reference counter of next mapper, if any
    if (m->next != NULL)
        if (--m->next->refcount == 0)
            mapper_final(m->next);

    // deallocate node of output list maintained by this mapper
    // if refcount == -1 then the mapper is a generator and has no
    // associated node
    if (m->refcount == -1) *m->out = NULL;
    else free(container_of(m->out, rnode_t, next));

    // dispose of mapper object 
    free(m);

    #if DEBUG > 1
    printf("[mapper %p]    destroyed\n", m);
    #endif

    // update global object count
    if (--objcount == 0) {
        #if DEBUG > 1
        printf("[mapper] ** disposing of hash table\n");
        #endif
        g_hash_table_destroy(hash);
        hash = NULL;
    }
}


// ---------------------------------------------------------------------
//  mapper_cons
// ---------------------------------------------------------------------
static void mapper_cons(mapper_t* m) {

    // pointer to next mapper, or NULL if input list is empty
    mapper_t* next;

    #if DEBUG > 0
    printf("[mapper %p] -- cons execution for list head: ", m);
    if (m->refcount == 0) printf("DEAD\n");
    else if (*m->in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*m->in)->val);
    #endif
    
    // if reference count of mapper has dropped to zero, do nothing
    // (mapper m will be removed by final handler)
    if (m->refcount == 0) {
        #if DEBUG > 1
        printf("[mapper %p]    mapper dead (refcount==0), error\n", m);
        #endif
        exit(1);
    }

    // get pointer to mapper associated with head node of input list
    if (*m->in == NULL) next = NULL;
    else {

        // find mapper associated with head node of input list
        next = g_hash_table_lookup(hash, &(*m->in)->next);

        #if DEBUG > 1
        printf("[mapper %p]    next mapper lookup yielded %p\n", m, next);
        #endif

        // if no mapper is known for the node, then create new one
        if (next == NULL) {

            // create head node of output list
            *m->out = (rnode_t*)malloc(sizeof(rnode_t));
            if (*m->out == NULL) {
                puts("[mapper]    out of memory");
                exit(1);
            }

            // create new mapper
            next = mapper_create(&(*m->in)->next, &(*m->out)->next, m->map, 0);
            if (next == NULL) {
                puts("[mapper]    out of memory");
                exit(1);
            }
        }
    }

    // if next mapper has changed
    if (m->next != next) {

        #if DEBUG > 1
        printf("[mapper %p]    relinking from %p to %p\n", m, m->next, next);
        #endif
        
        // increase reference counter of new next mapper, if any
        if (next != NULL) 
            if (next->refcount++ == 0) 
                schedule_final(next->cons_id, NULL);

        // decrease reference counter of old next mapper, if any
        if (m->next != NULL) {

            if (--m->next->refcount == 0)
                schedule_final(m->next->cons_id, (cons_t)mapper_final);            

            #if DEBUG > 1
            printf("[mapper %p]    decreased to %d reference counter of mapper %p\n",
                m, m->next->refcount, m->next);
            #endif
        }

        // link m to new mapper
        m->next = next;
    }

    // update output list
    if (next != NULL) {
        container_of(next->out, rnode_t, next)->val = m->map((*m->in)->val);
        *m->out = container_of(next->out, rnode_t, next);
    } else *m->out = NULL;
}


// ---------------------------------------------------------------------
// mapper_new
// ---------------------------------------------------------------------
mapper_t* mapper_new(rnode_t** in, rnode_t** out, map_t map) {
    
    // there can be only one mapper associated with head of input list
    if (hash != NULL && g_hash_table_lookup(hash, in) != NULL) 
        return NULL;

    return mapper_create(in, out, map, 1);
}


// ---------------------------------------------------------------------
// mapper_delete
// ---------------------------------------------------------------------
void mapper_delete(mapper_t* m) {
    
    // schedule generator node for deletion
    m->refcount = -1;
    mapper_final(m);
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
