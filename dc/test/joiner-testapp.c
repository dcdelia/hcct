/* ============================================================================
 *  joiner-testapp.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 28, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 09:26:31 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#include <stdio.h>
#include <stdlib.h>

#include "dc.h"
#include "rlist.h"
#include "joiner.h"


#define MAX_VAL         5
#define MAKE_DUMP_FILE  0


// ---------------------------------------------------------------------
//  check
// ---------------------------------------------------------------------
int check(rnode_t* in1, rnode_t* in2, rnode_t* out) {

    while (out != NULL && in1 != NULL) {
        if (out->val != in1->val) return 0;
        out = out->next;
        in1 = in1->next;
    }

    while (out != NULL && in2 != NULL) {
        if (out->val != in2->val) return 0;
        out = out->next;
        in2 = in2->next;
    }

    return out == NULL && in1 == NULL && in2 == NULL;
}


// ---------------------------------------------------------------------
//  dump
// ---------------------------------------------------------------------
void dump(rnode_t** in1, rnode_t** in2, rnode_t** out, int* count) {
    unsigned new_count = dc_num_exec_cons();
    printf("in1:  "); rlist_print(*in1);
    printf("in2:  "); rlist_print(*in2);
    printf("out: ");  rlist_print(*out);
    printf("exec cons: %u\n", new_count - *count);
    printf("correctness: %s\n", 
        check(*in1, *in2, *out) ? "ok" : "***failed***");
    *count = new_count;
}


// ---------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------
int main() {

    int count = 0; // number of executed constraints

    puts("--starting program");
    rnode_t **in1 = (rnode_t**)rmalloc(sizeof(rnode_t*));
    rnode_t **in2 = (rnode_t**)rmalloc(sizeof(rnode_t*));
    rnode_t **out = (rnode_t**)rmalloc(sizeof(rnode_t*));

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/joiner-start.dump");
    #endif

    srand(1371);

    *in1 = rlist_new_rand(10, MAX_VAL);
    *in2 = rlist_new_rand(20, MAX_VAL);
    *out = NULL;

    puts("--initial list in/out");
    dump(in1, in2, out, &count);

    puts("--making joiner");
    joiner_t* h = joiner_new(in1, in2, out);
    if (h == NULL) {
        printf("error: cannot initialize joiner\n");
        exit(0);
    }
    dump(in1, in2, out, &count);

    puts("\n--changing first item of in1");
    (*in1)->val = rand()%MAX_VAL;
    dump(in1, in2, out, &count);

    puts("\n--removing first item from in1");
    rlist_remove_first(in1);
    dump(in1, in2, out, &count);

    puts("\n--removing three items from in1 (batch)");
    begin_at();
    rlist_remove_first(in1);
    rlist_remove_first(in1);
    rlist_remove_first(in1);
    end_at();
    dump(in1, in2, out, &count);

    puts("\n--removing three items from in2 (batch)");
    begin_at();
    rlist_remove_first(in2);
    rlist_remove_first(in2);
    rlist_remove_first(in2);
    end_at();
    dump(in1, in2, out, &count);

    puts("\n--adding two items to in2 (batch)");
    begin_at();
    rlist_add_first(in2, rand()%MAX_VAL);
    rlist_add_first(in2, rand()%MAX_VAL);
    end_at();
    dump(in1, in2, out, &count);

    puts("\n--adding/removing items to list in1 (batch)");
    begin_at();
    rlist_remove_first(in1);
    rlist_add_first(in1, rand()%MAX_VAL);
    rlist_add_first(in1, rand()%MAX_VAL);
    end_at();
    dump(in1, in2, out, &count);

    puts("\n--replacing entire list in1");
    rnode_t* b = *in1;
    *in1 = rlist_new_rand(5, MAX_VAL);
    dump(in1, in2, out, &count);

    puts("\n--deleting entire list in1");
    rnode_t* c = *in1;
    *in1 = NULL;
    rlist_remove_all(&c);
    dump(in1, in2, out, &count);

    puts("\n--restoring previous list in1");
    *in1 = b;
    dump(in1, in2, out, &count);

    puts("\n--individually removing three items past the fourth node of in2");
    rlist_remove_first(&(*in2)->next->next->next->next);
    rlist_remove_first(&(*in2)->next->next->next->next);
    rlist_remove_first(&(*in2)->next->next->next->next);
    dump(in1, in2, out, &count);

    puts("\n--append to in1 a list of 5 new elements");
    rnode_t* last = rlist_get_last_node(*in1);
    last->next = rlist_new_rand(5, MAX_VAL);
    dump(in1, in2, out, &count);

    puts("\n--change value of 6th element from tail of list in1");
    last->val = rand()%MAX_VAL;
    dump(in1, in2, out, &count);

    puts("\n--swap two portions of list in1");
    begin_at();
    rlist_get_last_node(last)->next = *in1;
    *in1 = last->next;
    last->next = NULL;
    end_at();
    dump(in1, in2, out, &count);

    puts("\n--disposing of joiner");
    joiner_delete(h);
    dump(in1, in2, out, &count);

    puts("\n--clearing input lists in1 and in2");
    rlist_remove_all(in1);
    rlist_remove_all(in2);
    dump(in1, in2, out, &count);

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/joiner-end.dump");
    #endif

    rfree(in1);
    rfree(in2);
    rfree(out);

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
