/* ============================================================================
 *  adder-testapp.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 10, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/10 14:01:21 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/


#include <stdio.h>
#include <stdlib.h>

#include "dc.h"
#include "rlist.h"
#include "adder.h"


#define MAX_VAL         5
#define MAKE_DUMP_FILE  0


// ---------------------------------------------------------------------
//  dump
// ---------------------------------------------------------------------
void dump(rnode_t** in, int* sum, int* count) {
    unsigned new_count = dc_num_exec_cons();
    printf("in:  "); rlist_print(*in);
    printf("sum: %d\n", *sum);
    rnode_t* p;
    int x = 0;
    for (p = *in; p!=NULL; p=p->next) x += p->val;
    printf("correct sum: %d (%s)\n", x, x == *sum ? "ok" : "**failed**");
    printf("exec cons: %u\n", new_count - *count);
    *count = new_count;
}


// ---------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------
int main() {

    int count = 0; // number of executed constraints

    puts("--starting program");
    rnode_t **in  = (rnode_t**)rmalloc(sizeof(rnode_t*));
    int *sum = (int*)malloc(sizeof(int));

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/adder-start.dump");
    #endif

    srand(13);

    *in  = rlist_new_rand(20, MAX_VAL);
    *sum = 0;

    puts("--initial list in");
    dump(in, sum, &count);

    puts("--making adder");
    adder_t* h = adder_new(in, sum);
    if (h == NULL) {
        printf("error: cannot initialize adder\n");
        exit(0);
    }
    dump(in, sum, &count);

    puts("\n--changing first item of in");
    (*in)->val = rand()%MAX_VAL;
    dump(in, sum, &count);

    puts("\n--removing first item from in");
    rlist_remove_first(in);
    dump(in, sum, &count);

    puts("\n--removing three items from in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_remove_first(in);
    rlist_remove_first(in);
    end_at();
    dump(in, sum, &count);

    puts("\n--adding two items to in (batch)");
    begin_at();
    rlist_add_first(in, rand()%MAX_VAL);
    rlist_add_first(in, rand()%MAX_VAL);
    end_at();
    dump(in, sum, &count);

    puts("\n--adding/removing items to list in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_add_first(in, rand()%MAX_VAL);
    rlist_add_first(in, rand()%MAX_VAL);
    end_at();
    dump(in, sum, &count);

    puts("\n--replacing entire list in");
    rnode_t* b = *in;
    *in = rlist_new_rand(5, MAX_VAL);
    dump(in, sum, &count);

    puts("\n--deleting entire list in");
    rnode_t* c = *in;
    *in = NULL;
    rlist_remove_all(&c);
    dump(in, sum, &count);

    puts("\n--restoring previous list in");
    *in = b;
    dump(in, sum, &count);

    puts("\n--individually removing three items past the fourth node of in");
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    dump(in, sum, &count);

    puts("\n--append to in a list of 5 new elements");
    rnode_t* last = rlist_get_last_node(*in);
    last->next = rlist_new_rand(5, MAX_VAL);
    dump(in, sum, &count);

    puts("\n--change value of 6th element from tail of the list");
    last->val = rand()%MAX_VAL;
    dump(in, sum, &count);

    puts("\n--swap two portions of the list");
    begin_at();
    rlist_get_last_node(last)->next = *in;
    *in = last->next;
    last->next = NULL;
    end_at();
    dump(in, sum, &count);

    puts("\n--disposing of adder");
    adder_delete(h);
    dump(in, sum, &count);

    puts("\n--clearing input list");
    rlist_remove_all(in);
    dump(in, sum, &count);

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/adder-end.dump");
    #endif

    rfree(in);
    free(sum);

	printf ("--total number of cached instructions executed = %u\n", 
        g_stats.exec_cache_instr_count);

	printf ("--mem_read_count = %u\n", dc_stat.mem_read_count);
	printf ("--first_mem_read_count = %u\n", dc_stat.first_mem_read_count);
	printf ("--mem_write_count = %u\n", dc_stat.mem_write_count);
	printf ("--first_mem_write_count = %u\n", dc_stat.first_mem_write_count);

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
