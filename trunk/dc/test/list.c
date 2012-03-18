/* ============================================================================
 *  list.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/14 21:27:24 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.14 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "list.h"


// ---------------------------------------------------------------------
//  list_add_first
// ---------------------------------------------------------------------
void list_add_first(node_t** head, int val) {
    node_t* v = (node_t*)malloc(sizeof(node_t));
    v->val  = val;
    v->next = *head;
    *head = v;
}


// ---------------------------------------------------------------------
//  list_remove_first
// ---------------------------------------------------------------------
void list_remove_first(node_t** head) {
    if (*head == NULL) return;
    node_t* dead = *head;
    *head = dead->next;
    free(dead);
}


// ---------------------------------------------------------------------
//  list_remove_all
// ---------------------------------------------------------------------
void list_remove_all(node_t** head) {
    while (*head != NULL) list_remove_first(head);
}


// ---------------------------------------------------------------------
//  list_print
// ---------------------------------------------------------------------
void list_print(node_t* list) {
    for (; list!=NULL; list=list->next) printf("%d ", list->val);
    printf("\n");
}


// ---------------------------------------------------------------------
//  list_print_k
// ---------------------------------------------------------------------
void list_print_k(node_t* list, int k) {
    for (; list!=NULL && k>0; list=list->next, k--) printf("%d ", list->val);
    printf("\n");
}


// ---------------------------------------------------------------------
//  list_new_rand
// ---------------------------------------------------------------------
node_t* list_new_rand(int n, int max_val) {
    node_t* list = NULL;
    for (; n>0; n--) list_add_first(&list, rand()%max_val);
    return list;
}


// ---------------------------------------------------------------------
//  list_new_sorted_rand
// ---------------------------------------------------------------------
node_t* list_new_sorted_rand(int n, int max_delta) {
    node_t* list = NULL;
    int curr_val = n*max_delta;
    for (; n>0; n--) {
        curr_val -= (1 + rand() % max_delta);
        list_add_first(&list, curr_val);
    }
    return list;
}


// ---------------------------------------------------------------------
//  list_ins_rem_updates
// ---------------------------------------------------------------------
size_t list_ins_rem_updates(node_t** head) {
    size_t num_updates = 0;
    for (; *head != NULL; head = &(*head)->next) {
        int val = (*head)->val;
        list_remove_first(head);
        list_add_first(head, val);
        num_updates+=2;
    }
    return num_updates;
}


// ---------------------------------------------------------------------
//  list_merge
// ---------------------------------------------------------------------
void list_merge(node_t* a, node_t* b, node_t** out) {
    *out = NULL;
    if (a == NULL && b == NULL) return;
    if (b == NULL || (a != NULL && a->val < b->val)) {
        list_add_first(out, a->val);
        list_merge(a->next, b, &(*out)->next);
    }
    else {
        list_add_first(out, b->val);
        list_merge(a, b->next, &(*out)->next);
    }
}


// ---------------------------------------------------------------------
//  list_is_sorted
// ---------------------------------------------------------------------
int list_is_sorted(node_t* l) {
    for (; l != NULL; l = l->next)
        if (l->next != NULL && l->val > l->next->val) return 0;
    return 1;
}


// ---------------------------------------------------------------------
//  list_length
// ---------------------------------------------------------------------
int list_length(node_t* l) {
    int len = 0;
    for (; l!=NULL; l=l->next) ++len;
    return len;
}

// ---------------------------------------------------------------------
//  list_map
// ---------------------------------------------------------------------
node_t* list_map (node_t* list, list_map_t map)
{
    node_t *out_list, *curr_node;
            
    if (list==NULL)
        return NULL;
    
    out_list = (node_t*)malloc (sizeof (node_t));
    
    curr_node = out_list;
    
    curr_node->val = map (list->val);
    
    while (list->next != NULL)
    {
        list=list->next;
        curr_node->next=(node_t*)malloc (sizeof (node_t));
        curr_node=curr_node->next;
        curr_node->val=map (list->val);
    }
    
    curr_node->next=NULL;
    return out_list;
}

// ---------------------------------------------------------------------
//  list_sum
// ---------------------------------------------------------------------
int list_sum (node_t* l)
{
    int sum = 0;
    for (; l!=NULL; l=l->next) 
        sum=sum+l->val;
    
    return sum;
}

// ---------------------------------------------------------------------
//  list_halve
// ---------------------------------------------------------------------
void list_halve (node_t *inList, node_t** outList1, node_t** outList2)
{
    *outList1 = NULL;
    *outList2 = NULL;
    
    for (; inList != NULL; inList = inList->next)
        if (rand () % 2)
            list_add_first (outList1, inList->val);
        else
            list_add_first (outList2, inList->val);
}

// ---------------------------------------------------------------------
//  list_split
// ---------------------------------------------------------------------
void list_split (node_t *inList, node_t** outList1, node_t** outList2, int* pivot)
{
    *outList1 = NULL;
    *outList2 = NULL;
    
    for (; inList != NULL; inList = inList->next)
        if (inList->val <= *pivot)
            list_add_first (outList1, inList->val);
        else
            list_add_first (outList2, inList->val);
}

/* Copyright (C) 2010 Camil Demetrescu, Andrea Ribichini

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
