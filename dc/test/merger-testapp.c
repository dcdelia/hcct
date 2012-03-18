/* ============================================================================
 *  test-merger.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/25 14:56:02 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.6 $
*/


#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "list.h"
#include "rlist.h"
#include "merger.h"

#define MAX_DELTA 10

void dump(rnode_t **a, rnode_t **b, rnode_t **out) {
    #if 0
    dc_dump_arcs(a);
    dc_dump_arcs(b);
    dc_dump_arcs(out);
    #endif
    printf(">> number of constraints: %lu\n", dc_num_cons());
    printf(">> list a (*a=%p - a=%p) - ", *a, a);
    rlist_print(*a);
    printf(">> list b (*b=%p - b=%p) - ", *b, b);
    rlist_print(*b);
    printf(">> list out (*out=%p - out=%p) - ", *out, out);
    rlist_print(*out);
}

int main() {

    rnode_t **a, **b, **out;

    a   = (rnode_t**)rmalloc(sizeof(rnode_t*));
    b   = (rnode_t**)rmalloc(sizeof(rnode_t*));
    out = (rnode_t**)rmalloc(sizeof(rnode_t*));

    *a = *b = *out = NULL;

    rm_make_dump_file("logs/merger-start.dump");

    srand(1052);

    *a = rlist_new_sorted_rand(10, MAX_DELTA);
    *b = rlist_new_sorted_rand(10, MAX_DELTA);

    merger_new(a, b, out);
    dump(a, b, out);

    puts("\n--changing first item of b");
    (*b)->val--;
    dump(a, b, out);

    #if 1
    puts("\n--changing second item of b");
    (*b)->next->val+=5;
    dump(a, b, out);
    #endif

    #if 1
    puts("\n--removing first item of b");
    rlist_remove_first(b);
    dump(a, b, out);
    #endif

    #if 1
    puts("\n--adding item to a");
    rlist_add_first(a, (*a)->val - 1);
    dump(a, b, out);
    #endif

    #if 1
    puts("\n--adding item to b");
    rlist_add_first(b, (*b)->val - 1);
    dump(a, b, out);
    #endif

    rm_make_dump_file("logs/merger-end.dump");

    #if 1
    #define N 30
    printf("conventional list merge test:\n");
    node_t *x, *y, *z;
    x = list_new_sorted_rand(N/2+N/8, MAX_DELTA);
    y = list_new_sorted_rand(N-N/2-N/8, MAX_DELTA);
    list_merge(x, y, &z);
    list_print(x); 
    list_print(y); 
    list_print(z);
    list_remove_all(&x);
    list_remove_all(&y);
    list_remove_all(&z);
    #endif

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
