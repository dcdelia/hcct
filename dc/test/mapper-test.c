/* ============================================================================
 *  mapper-test.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 15:13:04 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.10 $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "dc.h"
#include "list.h"
#include "rlist.h"
#include "mapper.h"
#include "test-driver.h"


// maximum difference between consecutive values in the input list
//#define MAX_DELTA   20

// size of a = input_size - input_size*SPLIT_RATIO
// size of b = input_size*SPLIT_RATIO
//#define SPLIT(x,y)      ((double)(x)/((x)+(y)))
//#define SPLIT_A(n,x,y)  ((size_t)((n)*SPLIT(x,y)))
//#define SPLIT_B(n,x,y)  (((n)-(size_t)((n)*SPLIT(x,y))))


// specific test object
struct test_s {
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    int         check;
    const char* input_family;
    
    #if CONV || CHECK
    node_t*     a;
    node_t*     out;
    #endif

    rnode_t**   ra;
    rnode_t**   rout;

    mapper_t*   mapper;
};


// ---------------------------------------------------------------------
// make_test
// ---------------------------------------------------------------------
test_t* make_test(size_t input_size, int seed, 
                 const char* input_family, int check_correctness) {

    test_t* test = (test_t*)malloc(sizeof(test_t));
    if (test == NULL) return NULL;

    test->input_size = input_size;
    test->seed = seed;
    test->num_updates = 0;
    test->input_family = input_family;
    test->check = check_correctness;

    #if CONV || CHECK
    test->a = test->out = NULL;
    #endif

    return test;
}


// ---------------------------------------------------------------------
// del_test
// ---------------------------------------------------------------------
void del_test(test_t* test) {
/*
    mapper_delete(test->mapper);

    rlist_remove_all(test->ra);
    rlist_remove_all(test->rout);

    rfree(test->ra);
    rfree(test->rout);

    #if CONV || CHECK
    list_remove_all(&test->a);
    list_remove_all(&test->out);
    #endif

    free(test);*/
}


// ---------------------------------------------------------------------
// get_num_updates
// ---------------------------------------------------------------------
size_t get_num_updates(test_t* test) {
    return test->num_updates;
}


// ---------------------------------------------------------------------
// get_test_name
// ---------------------------------------------------------------------
char* get_test_name() {
    return "mapper-dc";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) {

    //int side_a, side_b;

    // allocate reactive list heads
    test->ra   = (rnode_t**)rmalloc(sizeof(rnode_t*));
    test->rout = (rnode_t**)malloc(sizeof(rnode_t*));

    if (test->ra == NULL || test->rout == NULL) 
        return "cannot initialize list heads";

    // initialize random generator
    srand(test->seed);

/*
    if (!strcmp("1-1", test->input_family)) 
        side_a = side_b = 1;
    else if (!strcmp("1-2", test->input_family)) 
        side_a = 1, side_b = 2;
    else if (!strcmp("1-3", test->input_family)) 
        side_a = 1, side_b = 3;
    else return "unknown family type";
*/

    // creates random list... 
    *test->ra = rlist_new_rand (test->input_size, INT_MAX);
        
    if (*test->ra == NULL) 
        return "can't build reactive input list";

    //printf("### updatable list length %d\n", rlist_length(*test->ra));

    // make checks
    #if CHECK
    if (test->check) {
        #if 0
        char* res;
        if (!rlist_is_sorted(*test->ra))
            return "reactive list a is not sorted";
        if (!rlist_is_sorted(*test->rb)) 
            return "reactive list b is not sorted";
        res = make_conv_input(test);
        if (res != NULL) return res;
        if (!rlist_equals_list(*test->ra, test->a))
            return "reactive list ra != list a";
        if (!rlist_equals_list(*test->rb, test->b)) 
            return "reactive list rb != list b";
        #endif
    }
    #endif

    // output list is initially empty
    *test->rout = NULL;

    return NULL;
}

int map (int i)
{
    int x=i;
    x = (x / 3) + (x / 7) + (x / 9);
    return x;
}

// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) {

    // makes mapper
    test->mapper = mapper_new(test->ra, test->rout, map);
    if (test->mapper == NULL) return "can't build mapper";

    // make checks
    #if CHECK
    if (test->check) {
        #if 0
        char* res = do_conv_eval(test);
        if (res != NULL) return res;
        if (!rlist_equals_list(*test->rout, test->out)) 
            return "reactive out list != conventional out list";
        list_remove_all(&test->out);
        #endif
    }
    #endif

    return NULL;
}


// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) {

    rnode_t** head=test->ra;
    
    //flag
    int rnd_updates=0;
    
    int j, k;
    
    test->num_updates = 0;
    //test->num_updates += rlist_ins_rem_updates(test->ra);
    //test->num_updates += rlist_ins_rem_updates(test->rb);

    if (!strncmp (test->input_family, "rnd-", 4))
    {
        rnd_updates=1;
        k=atoi (&(test->input_family[4]));
    }
    
    //ins_rem test...
    if (rnd_updates==0)
    {
        for (; *head != NULL; head = &(*head)->next) {
            //int val = (*head)->val;
            rlist_add_first(head, rand ());
            rlist_remove_first(head);
            test->num_updates+=2;
        }

        // make checks
        #if CHECK
        if (test->check) {
            #if 0
            char* res = do_conv_eval(test);
            if (res != NULL) return res;
            if (!rlist_equals_list(*test->rout, test->out)) 
                return "reactive out list != conventional out list";
            list_remove_all(&test->out);
            #endif
        }
        #endif
    }
    
    //random updates in batches of k...
    if (rnd_updates==1)
    {
        //cycles through list 3 times...
        for (j=0; j<3; j++)
        {
            int num_changes=0;
            //printf ("num_changes set to 0\n");
            //begin_at ();
            //printf ("begin_at\n");
            
            //runs entire list...
            for (head=test->ra; *head != NULL; head = &(*head)->next)
            {   
                if ((rand ()%2)==0)
                {
                    rlist_add_first (head, rand ());
                    //printf ("Inserted new item...");
                }
                else
                {
                    rlist_remove_first (head);
                    //printf ("Removed item...");
                }
                
                num_changes++;
                //printf ("num_changes = %d\n", num_changes);
                if (num_changes==k)
                {
                    num_changes=0;
                    //printf ("num_changes reset to 0\n");
                    //printf ("right before end_at\n");
                    //end_at ();
                    //printf ("end_at\n");
                    test->num_updates++;
                    //begin_at ();
                    //printf ("begin_at");
                }
            }
            //end_at ();
            if (num_changes!=0)
                test->num_updates++;
        }
    }
    
    return NULL;
}


#if CONV || CHECK

// ---------------------------------------------------------------------
// make_conv_input
// ---------------------------------------------------------------------
char* make_conv_input(test_t* test) {

#if 0
    int side_a, side_b;

    // initialize random generator
    srand(test->seed);

    if (!strcmp("1-1", test->input_family)) 
        side_a = side_b = 1;
    else if (!strcmp("1-2", test->input_family)) 
        side_a = 1, side_b = 2;
    else if (!strcmp("1-3", test->input_family)) 
        side_a = 1, side_b = 3;
    else return "unknown family type";

    // create random sorted lists with size ratio side_a:side_b
    test->a = 
        list_new_sorted_rand(
            SPLIT_A(test->input_size, side_a, side_b), MAX_DELTA);
    test->b = 
        list_new_sorted_rand(
            SPLIT_B(test->input_size, side_a, side_b), MAX_DELTA);
    if (test->a == NULL || test->b == NULL) 
        return "cannot create conventional input lists";

    // make checks
    #if CHECK
    if (test->check) {
        #if 0
        if (!list_is_sorted(test->a))
            return "conventional list a is not sorted";
        if (!list_is_sorted(test->b)) 
            return "conventional list b is not sorted";
        #endif
    }
    #endif
    
    // output list is initially empty
    test->out = NULL;    
#endif
    return NULL;
}


// ---------------------------------------------------------------------
// do_conv_eval
// ---------------------------------------------------------------------
char* do_conv_eval(test_t* test) {
#if 0
    list_merge(test->a, test->b, &test->out);
    if (test->check) {
        if (!list_is_sorted(test->out))
            return "conventional output list is not sorted";
    }
    #if 0
    list_print_k(test->a, 10);
    list_print_k(test->b, 10);
    list_print_k(test->out, 10);
    #endif
#endif
    return NULL;
}

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
