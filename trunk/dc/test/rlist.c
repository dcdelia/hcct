/* ============================================================================
 *  rlist.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/15 12:32:24 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.6 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "rlist.h"


// ---------------------------------------------------------------------
//  rlist_add_first
// ---------------------------------------------------------------------
void rlist_add_first(rnode_t** head, int val) {
    rnode_t* v = (rnode_t*)rmalloc(sizeof(rnode_t));
    v->val  = val;
    v->next = *head;
    *head = v;
}


// ---------------------------------------------------------------------
//  rlist_remove_first
// ---------------------------------------------------------------------
void rlist_remove_first(rnode_t** head) {
    if (*head == NULL) return;
    rnode_t* dead = *head;
    *head = dead->next;
    rfree(dead);
}


// ---------------------------------------------------------------------
//  rlist_remove_all
// ---------------------------------------------------------------------
void rlist_remove_all(rnode_t** head) {
    begin_at();
    while (*head != NULL) rlist_remove_first(head);
    end_at();
}


// ---------------------------------------------------------------------
//  rlist_remove_smaller
// ---------------------------------------------------------------------
void rlist_remove_smaller(rnode_t** head, int val) {
    begin_at();
    while (*head != NULL && (*head)->val < val)
        rlist_remove_first(head);
    end_at();
}


// ---------------------------------------------------------------------
//  rlist_print
// ---------------------------------------------------------------------
void rlist_print(rnode_t* rlist) {
    for (; rlist!=NULL; rlist=rlist->next) printf("%d ", rlist->val);
    printf("\n");
}


// ---------------------------------------------------------------------
//  rlist_print_k
// ---------------------------------------------------------------------
void rlist_print_k(rnode_t* list, int k) {
    for (; list!=NULL && k>0; list=list->next, k--) printf("%d ", list->val);
    printf("\n");
}


// ---------------------------------------------------------------------
//  rlist_new_rand
// ---------------------------------------------------------------------
rnode_t* rlist_new_rand(int n, int max_val) {
    rnode_t* rlist = NULL;
    for (; n>0; n--) rlist_add_first(&rlist, rand()%max_val);
    return rlist;
}


// ---------------------------------------------------------------------
//  rlist_new_sorted_rand
// ---------------------------------------------------------------------
rnode_t* rlist_new_sorted_rand(int n, int max_delta) {
    rnode_t* rlist = NULL;
    int curr_val = n*max_delta;
    for (; n>0; n--) {
        curr_val -= (1 + rand() % max_delta);
        rlist_add_first(&rlist, curr_val);
    }
    return rlist;
}


// ---------------------------------------------------------------------
//  rlist_ins_rem_updates
// ---------------------------------------------------------------------
size_t rlist_ins_rem_updates(rnode_t** head) {
    size_t num_updates = 0;
    for (; *head != NULL; head = &(*head)->next) {
        int val = (*head)->val;
        rlist_remove_first(head);
        rlist_add_first(head, val);
        num_updates+=2;
    }
    return num_updates;
}


// ---------------------------------------------------------------------
//  rlist_is_sorted
// ---------------------------------------------------------------------
int rlist_is_sorted(rnode_t* l) {
    for (; l != NULL; l = l->next)
        if (l->next != NULL && l->val > l->next->val) return 0;
    return 1;
}


// ---------------------------------------------------------------------
//  rlist_equals_list
// ---------------------------------------------------------------------
int rlist_equals_list(rnode_t* rl, node_t* l) {
    for (; rl != NULL && l != NULL; rl=rl->next, l=l->next)
        if (l->val != rl->val) return 0;
    if (rl != NULL || l != NULL) return 0;
    return 1;
}


// ---------------------------------------------------------------------
//  rlist_length
// ---------------------------------------------------------------------
int rlist_length(rnode_t* l) {
    int len = 0;
    for (; l!=NULL; l=l->next) ++len;
    return len;
}


// ---------------------------------------------------------------------
//  rlist_get_last_node
// ---------------------------------------------------------------------
rnode_t* rlist_get_last_node(rnode_t* l){
    rnode_t* prev = NULL;
    for (; l!=NULL; l=l->next) prev = l;
    return prev;
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
