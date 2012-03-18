/* ============================================================================
 *  mapper.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 28, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/03 08:48:38 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.2 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"
#include "dc.h"

// macros
#define DEBUG 0

// mapper object
struct mapper_s {
    rnode_t** in;       // input list
    rnode_t** out;      // output list
    map_t     map;      // map function
    mapper_t* next;     // pointer to next mapper
    int cons_id;        // constraint associated with output node
};


// ---------------------------------------------------------------------
// mapper_cons
// ---------------------------------------------------------------------
static void mapper_cons(mapper_t* m) {

    rnode_t  *in = *m->in,   *pin;   // input list node cursors
    mapper_t *sh =  m->next, *psh;   // mapper node cursors

    #if DEBUG > 0
    printf("[mapper %p] aligning heads &in=%p &out=%p - ", m, m->in, m->out); 
    if (*m->in  == NULL) printf("in=nil");     else printf("in=%d",(*m->in)->val);
    if (*m->out == NULL) printf(" out=nil\n"); else printf(" out=%d\n",(*m->out)->val);
    #endif

    // if one or both lists are empty, skip realignment phase
    if (in == NULL) { sh = NULL; goto remove; }
    if (sh == NULL) { in = NULL; goto add;    }

    // efficiently realign mapper list in case of monotonical update 
    do {
        if (&in->next ==  m->next->in) goto add;
        if (&(*m->in)->next == sh->in) { in = *m->in;  goto remove; }
        in = in->next, sh = sh->next;
    } while(in != NULL && sh != NULL);

    // when one list gets empty, continue with the other one
    for (; in != NULL; in = in->next)
        if (&in->next ==  m->next->in) goto add;
    for (; sh != NULL; sh = sh->next)
        if (&(*m->in)->next == sh->in) { in = *m->in;  goto remove; }

    #if DEBUG > 0
    if (in == NULL || sh == NULL) 
        printf("[mapper %p] realignment failed, rebuild from scratch\n", m); 
    #endif

  remove: // remove nodes from out and mapper lists
    while (m->next != sh) {        

        #if DEBUG > 0
        printf("[mapper %p] deleting %p\n", m, m->next); 
        #endif

        // remove first node from out 
        rlist_remove_first(m->out);

        // remove first node of mapper list
        mapper_t* dead = m->next;
        m->next = dead->next;
        delcons(dead->cons_id);
        free(dead);
    }

    // if input list is empty, bail out
    if (*m->in == NULL) return;

  add: // add new nodes to both out and mapper lists
    for (pin = *m->in, psh = m; 
         pin != in;
         pin = pin->next, psh = psh->next) {

        #if DEBUG > 0
        printf("[mapper %p] adding node %p with val %d to out\n", m, pin, m->map(pin->val)); 
        #endif

        // add new node to mapper list and to out list
        mapper_t* tail = psh->next;
        rlist_add_first(psh->out, m->map(pin->val));
        psh->next = mapper_new(&pin->next, &(*psh->out)->next, m->map);
        psh->next->next = tail;
    }

    // synchronize out value of head node with in value
    (*m->out)->val = m->map((*m->in)->val);
    __asm__ __volatile__ ( "nop;nop;nop;" );
}


// ---------------------------------------------------------------------
// mapper_new
// ---------------------------------------------------------------------
mapper_t* mapper_new(rnode_t** in, rnode_t** out, map_t map) {

    // allocate mapper object
    mapper_t* m = (mapper_t*)malloc(sizeof(mapper_t));

    // initialize it
    m->in      = in;
    m->out     = out;
    m->map     = map;
    m->next    = NULL;
    m->cons_id = newcons((cons_t)mapper_cons, m);

    return m;
}


// ---------------------------------------------------------------------
// mapper_delete
// ---------------------------------------------------------------------
void mapper_delete(mapper_t* m) {
	delcons(m->cons_id);
    if (m->next != NULL) mapper_delete(m->next);
    free(m);
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
