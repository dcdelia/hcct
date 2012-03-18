/* ============================================================================
 *  merger.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 15:13:05 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.2 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "merger.h"
#include "dc.h"


#ifndef DEBUG
#define DEBUG  2
#endif


// merger structure
struct merger_s {
    rnode_t **a, **b, **out;
    int cons_id;
    struct merger_s* next;
};


// required prototypes
static void merger_cons(merger_t* m);


// ---------------------------------------------------------------------
//  merger_alloc
// ---------------------------------------------------------------------
static merger_t* merger_alloc(rnode_t** a, rnode_t** b, rnode_t** out, merger_t* next) {
    merger_t* m = (merger_t*)malloc(sizeof(merger_t));
    m->a = a;
    m->b = b;
    m->out = out;
    m->next = next;
    m->cons_id = newcons((cons_t)merger_cons, m);
    #if DEBUG > 0
    printf(" - merger %p created with heads &a=%p &b=%p &out=%p - ", m, m->a, m->b,  m->out); 
    if (*m->a == NULL) printf("a=nil"); else printf("a=%d",(*m->a)->val);
    if (*m->b == NULL) printf(" b=nil"); else printf(" b=%d",(*m->b)->val);
    if (*m->out == NULL) printf(" out=nil\n"); else printf(" out=%d\n",(*m->out)->val);
    #endif
    return m;
}


// ---------------------------------------------------------------------
//  merger_free
// ---------------------------------------------------------------------
static void merger_free(merger_t* m) {

    #if DEBUG > 1
    printf(" - merger %p deleted\n", m);
    #endif

    delcons(m->cons_id);
    free(m);
}


// ---------------------------------------------------------------------
//  remove_all
// ---------------------------------------------------------------------
static void remove_all(merger_t* m) {
    while (m->next != NULL) {
        merger_t* dead = m->next;
        list_remove_first((node_t**)m->out);
        m->next = dead->next;
        merger_free(dead);
    }
}


// ---------------------------------------------------------------------
//  remove_smaller
// ---------------------------------------------------------------------
static void remove_smaller(merger_t* m, int val) {
    while (m->next != NULL && (*m->out)->val < val) {
        merger_t* dead = m->next;
        list_remove_first((node_t**)m->out);
        m->next = dead->next;
        merger_free(dead);
    }
}


// ---------------------------------------------------------------------
//  merger_cons
// ---------------------------------------------------------------------
static void merger_cons(merger_t* m) {
    
    #if DEBUG > 0
    printf("[merger %p] merging heads &a=%p &b=%p &out=%p - ", m, m->a, m->b,  m->out); 
    if (*m->a == NULL) printf("a=nil"); else printf("a=%d",(*m->a)->val);
    if (*m->b == NULL) printf(" b=nil"); else printf(" b=%d",(*m->b)->val);
    if (*m->out == NULL) printf(" out=nil\n"); else printf(" out=%d\n",(*m->out)->val);
    #endif

    if (*m->a == NULL && *m->b == NULL) {
        #if DEBUG > 1
        printf(" - removing all items from output list\n");
        #endif
        remove_all(m);
        return;
    }

    if (*m->b == NULL || (*m->a != NULL && (*m->a)->val < (*m->b)->val)) {
        remove_smaller(m, (*m->a)->val);
        if (m->next == NULL || (*m->out)->val > (*m->a)->val) {
            list_add_first((node_t**)m->out, (*m->a)->val);
            m->next = merger_alloc(&(*m->a)->next, m->b, &(*m->out)->next, m->next);
        }
        else {
            m->next->a = &(*m->a)->next;
            m->next->b = m->b;
        }
    }
    else {
        remove_smaller(m, (*m->b)->val);
        if (m->next == NULL || (*m->out)->val > (*m->b)->val) {
            list_add_first((node_t**)m->out, (*m->b)->val);
            m->next = merger_alloc(m->a, &(*m->b)->next, &(*m->out)->next, m->next);
        }
        else {
            m->next->a = m->a;
            m->next->b = &(*m->b)->next;
        }
    }
}


// ---------------------------------------------------------------------
//  merger_new
// ---------------------------------------------------------------------
merger_t* merger_new(rnode_t** a, rnode_t** b, rnode_t** out) {
    return merger_alloc(a, b, out, NULL);
}


// ---------------------------------------------------------------------
//  merger_delete
// ---------------------------------------------------------------------
// deallocate merger list and detach from output list
void merger_delete(merger_t* m) {

    if (m->next != NULL) merger_delete(m->next);
    merger_free(m);
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
