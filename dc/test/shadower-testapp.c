/* ============================================================================
 *  test_shadower.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/25 13:32:22 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/


#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "rlist.h"
#include "shadower.h"

#define MAX_VAL 100


typedef struct {
    int cons_id;
} my_shadow_rec_t;


void con(shadow_node_t* shadow_node) {
    rnode_t* node = (rnode_t*)shadow_node_get_shadowed_node(shadow_node);
    printf("[main] node %p created with value %d\n", node, node->val);
}

void des(shadow_node_t* shadow_node) {
    printf("[main] node %p deleted\n", 
        shadow_node_get_shadowed_node(shadow_node));
}


void shadow_rlist_print(shadower_t* s) {
    printf(">> shadow list: ");
    shadow_node_t* p = shadower_get_first_shadow_node(s);
    shadow_node_t* l = shadower_get_last_shadow_node(s);
    if (l == NULL) 
         printf("(last=nil) "); 
    else printf("(last=%d) ", 
            ((rnode_t*)shadow_node_get_shadowed_node(l))->val);
    while (p!= NULL) {
        rnode_t* node = (rnode_t*)shadow_node_get_shadowed_node(p);
        printf("%d ", node->val);
        p = shadow_node_get_next(p);
    }
    printf("\n");
}


void shadow_reverse_rlist_print(shadower_t* s) {
    printf(">> reverse shadow list: ");
    shadow_node_t* l = shadower_get_last_shadow_node(s);
    while (l!= NULL) {
        rnode_t* node = (rnode_t*)shadow_node_get_shadowed_node(l);
        printf("%d ", node->val);
        l = shadow_node_get_prev(l);
    }
    printf("\n");
}


void dump(rnode_t **a, shadower_t* s) {
    #if 0
    dc_dump_arcs(a);
    dc_dump_arcs(b);
    dc_dump_arcs(out);
    #endif
    printf(">> number of constraints: %lu\n", dc_num_cons());
    printf(">> list a (*a=%p - a=%p) - ", *a, a);
    rlist_print(*a);
    if (s != NULL) {
        shadow_rlist_print(s);
        shadow_reverse_rlist_print(s);
    }
}


int main() {

    printf("initializing system...\n");

    rnode_t **a = (rnode_t**)rmalloc(sizeof(rnode_t));

    //rm_make_dump_file("logs/shadower-start.dump");

    srand(1052);

    *a = rlist_new_rand(10, MAX_VAL);

    puts("--initial list a");
    dump(a, NULL);

    shadower_t* s = 
        shadower_new((void**)a, offsetof(rnode_t, next),
                     sizeof(my_shadow_rec_t), 
                     con, des, NULL);
    if (s == NULL) {
        printf("error: cannot initialize shadower\n");
        exit(0);
    }

    puts("\n--changing first item of a");
    (*a)->val = rand()%MAX_VAL;
    dump(a, s);

    puts("\n--removing first item from a");
    rlist_remove_first(a);
    dump(a, s);

    puts("\n--removing three items from a (batch)");
    begin_at();
    rlist_remove_first(a);
    rlist_remove_first(a);
    rlist_remove_first(a);
    end_at();
    dump(a, s);

    puts("\n--adding two items to a (batch)");
    begin_at();
    rlist_add_first(a, rand()%MAX_VAL);
    rlist_add_first(a, rand()%MAX_VAL);
    end_at();
    dump(a, s);
    printf("%p-%p-%p\n", *a, (*a)->next, (*a)->next->next);

    puts("\n--adding/removing items to a (batch)");
    begin_at();
    rlist_remove_first(a);
    rlist_add_first(a, rand()%MAX_VAL);
    rlist_add_first(a, rand()%MAX_VAL);
    end_at();
    dump(a, s);
    printf("%p-%p-%p\n", *a, (*a)->next, (*a)->next->next);

    puts("\n--replacing entire list a");
    rnode_t* b = *a;
    *a = rlist_new_rand(5, MAX_VAL);
    dump(a, s);

    puts("\n--deleting entire list a");
    rnode_t* c = *a;
    *a = NULL;
    rlist_remove_all(&c);
    dump(a, s);

    puts("\n--restoring previous list a");
    *a = b;
    dump(a, s);

    puts("\n--individually removing three items past the fourth node of a");
    rlist_remove_first(&(*a)->next->next->next->next);
    rlist_remove_first(&(*a)->next->next->next->next);
    rlist_remove_first(&(*a)->next->next->next->next);
    dump(a, s);

    puts("\n--disposing of shadower");
    shadower_delete(s);

    rlist_remove_all(a);

    rm_make_dump_file("logs/shadower-end.dump");

	printf ("Total number of cached instructions executed so far = %u\n", g_stats.exec_cache_instr_count);

    return 0;
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
