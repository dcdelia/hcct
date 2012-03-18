/* ============================================================================
 *  list.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/14 21:27:25 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.13 $
*/


#ifndef __list__
#define __list__

typedef struct node_s {
    int val;
    struct node_s* next;
} node_t;

typedef int (*list_map_t)(int val);

void list_add_first(node_t** head, int val);
void list_remove_first(node_t** head);
void list_remove_all(node_t** head);
void list_print(node_t* list);
void list_print_k(node_t* list, int k);
node_t* list_new_rand(int n, int max_val);
node_t* list_new_sorted_rand(int n, int max_delta);
void list_merge(node_t* a, node_t* b, node_t** out);
int list_is_sorted(node_t* l);
int list_length(node_t* l);

node_t* list_map (node_t* list, list_map_t map);
int list_sum (node_t* list);
void list_halve (node_t *inList, node_t** outList1, node_t** outList2);
void list_split (node_t *inList, node_t** outList1, node_t** outList2, int* pivot);

#endif


/* Copyright (C) 2010 Camil Demetrescu, Andrea Ribichini

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
