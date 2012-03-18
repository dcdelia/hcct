/* ============================================================================
 *  shadower.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/23 07:53:56 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.7 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "shadower.h"
#include "dc.h"

// macros
#define DEBUG 1
#define MALLOC rmalloc
#define FREE rfree
#define NEXT_FIELD(node, off) (*(void**)((char*)(node)+(off)))

// prototypes
static void shadower_cons(shadow_node_t* n);
static void insert_shadow_node(shadow_node_t** shadow_list, 
                               shadower_t* shadower, 
                               void* shadowed_node, 
                               void** shadowed_list);
static void remove_shadow_node(shadow_node_t** shadow_list);


// ---------------------------------------------------------------------
// shadower_cons
// ---------------------------------------------------------------------
static void shadower_cons(shadow_node_t* n) {

    void          *a   = *n->next_shadowed_node, *pa;
    shadow_node_t *b   =  n->next_shadow_node, *pb, **head;
    size_t         off =  n->shadower->next_off;

    // update shadower's pointer to last node in shadow list
    if (a == NULL) n->shadower->last_shadow_node = n;

    // efficiently realign shadow list in case of monotonical update
    if (b != NULL)
        while (a != NULL || b != NULL) {
            if (a != NULL) {
                if (n->next_shadow_node->shadowed_node == a) {
                    #if DEBUG > 0
                    if (a == *n->next_shadowed_node)
                         printf("[shadow node %p] cons execution: head unchanged -- skip\n", n);
                    else printf("[shadow node %p] cons execution: head insertions only - efficient realign\n", n);
                    #endif
                    for ( pa = *n->next_shadowed_node, head = &n->next_shadow_node; 
                          pa != a; 
                          pa = NEXT_FIELD(pa,off), head = &(*head)->next_shadow_node)
                        insert_shadow_node(head, n->shadower, pa, &NEXT_FIELD(pa,off));
                    goto end;
                }
                a = NEXT_FIELD(a,off);
            }
    
            if (b != NULL) {
                if (b->shadowed_node == *n->next_shadowed_node) {
                    #if DEBUG > 0
                    printf("[shadow node %p] cons execution: head deletions only - efficient realign\n", n);
                    #endif
                    for (pb = n->next_shadow_node; pb != b; pb = pb->next_shadow_node)
                        remove_shadow_node(&n->next_shadow_node);
                    goto end;                
                }
                b = b->next_shadow_node;
            }
        }

    #if DEBUG > 0
    if (n->next_shadow_node == NULL && *n->next_shadowed_node == NULL)
         printf("[shadow node %p] cons execution: both lists are empty -- skip\n", n);
    else if (n->next_shadow_node == NULL)
         printf("[shadow node %p] cons execution: build shadow list from scratch\n", n);
    else if (*n->next_shadowed_node == NULL)
         printf("[shadow node %p] cons execution: delete shadow list\n", n);
    else printf("[shadow node %p] cons execution: non-monotonical update -- rebuild shadow list from scratch\n", n);
    #endif

    // if update was non-monotonical or shadow list was empty, 
    // then (re)create list from scratch
    while (n->next_shadow_node != NULL)
        remove_shadow_node(&n->next_shadow_node);
    for ( pa = *n->next_shadowed_node, head = &n->next_shadow_node; 
          pa != NULL; 
          pa = NEXT_FIELD(pa,off), head = &(*head)->next_shadow_node)
        insert_shadow_node(head, n->shadower, pa, &NEXT_FIELD(pa,off));

  end:

    // if n is not the generator node, let it successor, if any, point to it
    if (n->next_shadow_node != NULL) {
        if (n != n->shadower->generator) 
             n->next_shadow_node->prev_shadow_node = n;
        else n->next_shadow_node->prev_shadow_node = NULL;
    }
}


// ---------------------------------------------------------------------
// insert_shadow_node
// ---------------------------------------------------------------------
static void insert_shadow_node(shadow_node_t** shadow_list, 
                               shadower_t* shadower, 
                               void* shadowed_node, 
                               void** shadowed_list) {

    // create fat shadow node
    shadow_node_t* n = 
        (shadow_node_t*) MALLOC(sizeof(shadow_node_t)+shadower->rec_size);

    #if DEBUG > 1
    printf("[shadow node %p] created\n", n);
    #endif

    // initialize it
    n->shadowed_node      = shadowed_node;
    n->next_shadowed_node = shadowed_list;
    n->shadower           = shadower;
    n->prev_shadow_node   = NULL;

    // add it to shadow list
    n->next_shadow_node = *shadow_list;
    *shadow_list = n;

    // signal clients of node creation
    if (shadowed_node != NULL && shadower->con != NULL) 
        shadower->con(n);

    // associate constraint with shadow node
    n->cons_id = newcons((cons_t)shadower_cons, NULL, n);
}


// ---------------------------------------------------------------------
// remove_shadow_node
// ---------------------------------------------------------------------
static void remove_shadow_node(shadow_node_t** shadow_list) {

    shadow_node_t* dead = *shadow_list;

    #if DEBUG > 1
    printf("[shadow node %p] deleted\n", dead);
    #endif

    // free costraint associated with node
    delcons(dead->cons_id);

    // signal clients of node destruction
    if (dead->shadowed_node != NULL && dead->shadower->des != NULL) 
        dead->shadower->des(dead);

    // unlink node from shadow list
    *shadow_list = dead->next_shadow_node;

    // free node
    FREE(dead);    
}


// ---------------------------------------------------------------------
// shadower_new
// ---------------------------------------------------------------------
shadower_t* shadower_new(
        void** list_head, size_t next_off, size_t rec_size, 
        con_t con, des_t des, void* param) {

    if (!rm_is_reactive(list_head)) return NULL;

    // allocate shadower object
    shadower_t* s = (shadower_t*)MALLOC(sizeof(shadower_t));

    // initialize it
    s->generator = NULL;
    s->last_shadow_node = NULL;
    s->con = con;
    s->des = des;
    s->next_off = next_off;
    s->param = param;
    s->rec_size = rec_size;

    // create dummy generator node
    insert_shadow_node(&s->generator, s, NULL, list_head);

    return s;
}


// ---------------------------------------------------------------------
// shadower_delete
// ---------------------------------------------------------------------
void shadower_delete(shadower_t* s) {
    begin_at();
    while (s->generator) remove_shadow_node(&s->generator);
    end_at();
    FREE(s);
}


/* Copyright (C) 2010 Camil Demetrescu

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
