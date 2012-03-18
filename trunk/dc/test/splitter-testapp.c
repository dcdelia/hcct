/* ============================================================================
 *  splitter-testapp.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 5, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/14 13:14:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.6 $
*/


#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "rlist.h"
#include "splitter.h"

#define MAX_VAL         100
#define MAKE_DUMP_FILE  0


int check_splitter(rnode_t* in, rnode_t* out1, rnode_t* out2, int pivot) {

    rnode_t* p;

    // check if list lengths are correct
    printf("checking list lengths...\n");
    if (rlist_length(in) != rlist_length(out1)+rlist_length(out2)) {
        printf("error: incorrect list lengths\n");
        return 0;
    }

    // check if all nodes in out1 are correct
    printf("checking small list...\n");
    for (p = out1; p != NULL; p=p->next) 
        if (p->val > pivot) {
            printf("error: one item in small list is larger than pivot\n");
            return 0;
        }

    // check if all nodes in out2 are correct
    printf("checking large list...\n");
    for (p = out2; p != NULL; p = p->next) {
        if (p->val < pivot) {
            printf("error: one item in small list is larger than pivot\n");
            return 0;
        }
    }

    printf("all checks ok\n");

    return 1;
}


void dump(rnode_t** in, rnode_t** out1, rnode_t** out2, 
          int* count, int pivot) {

    unsigned new_count = dc_num_exec_cons();

    printf("in:    "); rlist_print(*in);
    printf("small: "); rlist_print(*out1);
    printf("pivot: %d\n", pivot);
    printf("large: "); rlist_print(*out2);

    if (count == NULL) return;

    if (check_splitter(*in, *out1, *out2, pivot))
         puts("operation ok");
    else puts("*** operation failed");

    printf("exec cons: %u\n", new_count - *count);

    *count = new_count;
}


int main() {

    int count = 0;  // number of executed constraints

    rnode_t **in   = (rnode_t**)rmalloc(sizeof(rnode_t));
    rnode_t **out1 = (rnode_t**)rmalloc(sizeof(rnode_t));
    rnode_t **out2 = (rnode_t**)rmalloc(sizeof(rnode_t));
    int* pivot = (int*)rmalloc(sizeof(int));
    *pivot = MAX_VAL / 2;

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/splitter-start.dump");
    #endif

    srand(3);

    *in = rlist_new_rand(20, MAX_VAL);
    *out1 = *out2 = NULL;

    puts("--initial list in");
    dump(in, out1, out2, NULL, *pivot);

    puts("--making splitter");
    splitter_t* h = splitter_new(in, out1, out2, pivot);
    if (h == NULL) {
        printf("error: cannot initialize splitter\n");
        exit(0);
    }
    dump(in, out1, out2, &count, *pivot);

    puts("\n--changing to 1 first item of in");
    (*in)->val = 1;
    dump(in, out1, out2, &count, *pivot);

    puts("\n--changing pivot");
    *pivot /= 2;
    dump(in, out1, out2, &count, *pivot);

    puts("\n--restoring original pivot");
    *pivot <<= 1;
    dump(in, out1, out2, &count, *pivot);

    puts("\n--removing first item from in (pivot)");
    rlist_remove_first(in);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--removing second item from in");
    rlist_remove_first(&(*in)->next);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--removing three items from in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_remove_first(in);
    rlist_remove_first(in);
    end_at();
    dump(in, out1, out2, &count, *pivot);

    puts("\n--adding four items {-1,-2,-3-,4} to in past the second item (batch)");
    begin_at();
    rlist_add_first(&(*in)->next->next, -4);
    //dump(in, out1, out2, &count);
    rlist_add_first(&(*in)->next->next, -3);
    //dump(in, out1, out2, &count);
    rlist_add_first(&(*in)->next->next, -2);
    //dump(in, out1, out2, &count);
    rlist_add_first(&(*in)->next->next, -1);
    end_at();
    dump(in, out1, out2, &count, *pivot);

    puts("\n--removing 1st item + adding {1,2} to in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_add_first(in, 2);
    rlist_add_first(in, 1);
    end_at();
    dump(in, out1, out2, &count, *pivot);

    puts("\n--replacing entire list in");
    rnode_t* b = *in;
    *in = rlist_new_rand(5, MAX_VAL);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--adding 33 to in");
    rlist_add_first(in, 33);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--deleting entire list in");
    rnode_t* c = *in;
    *in = NULL;
    rlist_remove_all(&c);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--restoring previous list in");
    *in = b;
    dump(in, out1, out2, &count, *pivot);

    puts("\n--individually removing three items past the fourth node of in");
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    dump(in, out1, out2, &count, *pivot);

    puts("\n--disposing of splitter");
    splitter_delete(h);

    puts("\n--clearing input list");
    rlist_remove_all(in);
    rlist_remove_all(out1);
    rlist_remove_all(out2);

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/splitter-end.dump");
    #endif

    rfree(in);
    rfree(out1);
    rfree(out2);
    rfree(pivot);

	printf ("total number of cached instructions executed so far = %u\n", 
        g_stats.exec_cache_instr_count);

	printf ("mem_read_count = %u\n", dc_stat.mem_read_count);
	printf ("first_mem_read_count = %u\n", dc_stat.first_mem_read_count);
	printf ("mem_write_count = %u\n", dc_stat.mem_write_count);
	printf ("first_mem_write_count = %u\n", dc_stat.first_mem_write_count);

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
