/* ============================================================================
 *  halver-testapp.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 27, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/05 22:38:01 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.5 $
*/


#include <stdio.h>
#include <stdlib.h>
#include "dc.h"
#include "rlist.h"
#include "halver.h"

#define MAX_VAL         100
#define MAKE_DUMP_FILE  0


int check_halve(rnode_t* in, rnode_t* out1, rnode_t* out2) {
    while (in != NULL) {
        if (out1 != NULL && out1->val == in->val)
            out1 = out1->next;
        else if (out2 != NULL && out2->val == in->val)
            out2 = out2->next;
        else return 0;
        in = in->next;
    }
    return out1 == NULL && out2 == NULL;
}

void dump(rnode_t** in, rnode_t** out1, rnode_t** out2, int* count) {
    unsigned new_count = dc_num_exec_cons();
    printf("in, out1, out2:\n");
    rlist_print(*in);
    rlist_print(*out1);
    rlist_print(*out2);
    if (check_halve(*in, *out1, *out2))
         puts("operation ok");
    else puts("*** operation failed");
    printf("exec cons: %u\n", new_count - *count);
    *count = new_count;
}


int main() {

    int count = 0; // number of executed constraints

    rnode_t **in   = (rnode_t**)rmalloc(sizeof(rnode_t));
    rnode_t **out1 = (rnode_t**)rmalloc(sizeof(rnode_t));
    rnode_t **out2 = (rnode_t**)rmalloc(sizeof(rnode_t));

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/halver-start.dump");
    #endif

    srand(11);

    *in = rlist_new_rand(20, MAX_VAL);
    *out1 = *out2 = NULL;

    puts("--initial list in");
    dump(in, out1, out2, &count);

    puts("--making halver");
    halver_t* h = halver_new(in, out1, out2);
    if (h == NULL) {
        printf("error: cannot initialize halver\n");
        exit(0);
    }
    dump(in, out1, out2, &count);

    puts("\n--changing first item of in");
    (*in)->val = rand()%MAX_VAL;
    dump(in, out1, out2, &count);

    puts("\n--removing first item from in");
    rlist_remove_first(in);
    dump(in, out1, out2, &count);

    puts("\n--removing first item from in");
    rlist_remove_first(in);
    dump(in, out1, out2, &count);

    puts("\n--removing three items from in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_remove_first(in);
    rlist_remove_first(in);
    end_at();
    dump(in, out1, out2, &count);

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
    dump(in, out1, out2, &count);

    puts("\n--removing 1st item + adding {1,2} to in (batch)");
    begin_at();
    rlist_remove_first(in);
    rlist_add_first(in, 2);
    rlist_add_first(in, 1);
    end_at();
    dump(in, out1, out2, &count);

    puts("\n--replacing entire list in");
    rnode_t* b = *in;
    *in = rlist_new_rand(5, MAX_VAL);
    dump(in, out1, out2, &count);

    puts("\n--deleting entire list in");
    rnode_t* c = *in;
    *in = NULL;
    rlist_remove_all(&c);
    dump(in, out1, out2, &count);

    puts("\n--restoring previous list in");
    *in = b;
    dump(in, out1, out2, &count);

    puts("\n--individually removing three items past the fourth node of in");
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    rlist_remove_first(&(*in)->next->next->next->next);
    dump(in, out1, out2, &count);

    puts("\n--disposing of halver");
    halver_delete(h);

    puts("\n--clearing input list");
    rlist_remove_all(in);
    rlist_remove_all(out1);
    rlist_remove_all(out2);

    #if MAKE_DUMP_FILE
    rm_make_dump_file("logs/halver-end.dump");
    #endif

    rfree(in);
    rfree(out1);
    rfree(out2);

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
