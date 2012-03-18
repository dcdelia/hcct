/* ============================================================================
 *  rlist.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/15 09:54:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.6 $
*/


#ifndef __rlist__
#define __rlist__

#include "list.h"
#include <stddef.h>

#define container_of(ptr, type, member) ( \
    (type*)((char*)ptr - offsetof(type, member)))

typedef struct rnode_s {
    int val;
    struct rnode_s* next;
} rnode_t;

void rlist_add_first(rnode_t** head, int val);
void rlist_remove_first(rnode_t** head);
void rlist_remove_all(rnode_t** head);
void rlist_remove_smaller(rnode_t** head, int val);
void rlist_print(rnode_t* list);
void rlist_inactive_print(rnode_t* list);
void rlist_print_k(rnode_t* list, int k);
rnode_t* rlist_new_rand(int n, int max_val);
rnode_t* rlist_new_sorted_rand(int n, int max_delta);
size_t rlist_ins_rem_updates(rnode_t** head);
int rlist_is_sorted(rnode_t* l);
int rlist_equals_list(rnode_t* rl, node_t* l);
int rlist_length(rnode_t* l);
rnode_t* rlist_get_last_node(rnode_t* l);

#endif


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
