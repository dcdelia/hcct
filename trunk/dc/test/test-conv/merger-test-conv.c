/* ============================================================================
 *  merger-test-conv.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 12:00:51 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.1 $
*/


#include <stdio.h>
#include <string.h>

#include "dc.h"
#include "list.h"
#include "merger.h"
#include "test-driver.h"


// maximum difference between consecutive values in the input list
#define MAX_DELTA   20

// size of a = input_size - input_size*SPLIT_RATIO
// size of b = input_size*SPLIT_RATIO
#define SPLIT(x,y)      ((double)(x)/((x)+(y)))
#define SPLIT_A(n,x,y)  ((size_t)((n)*SPLIT(x,y)))
#define SPLIT_B(n,x,y)  (((n)-(size_t)((n)*SPLIT(x,y))))


// specific test object
struct test_s {
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    int         check;
    const char* input_family;
    
    node_t*     a;
    node_t*     b;
    node_t*     out;

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

    test->a = test->b = test->out = NULL;

    return test;
}


// ---------------------------------------------------------------------
// del_test
// ---------------------------------------------------------------------
void del_test(test_t* test) {


    list_remove_all(&test->a);
    list_remove_all(&test->b);
    list_remove_all(&test->out);

    free(test);
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
    return "merger-conv";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) {

    int side_a, side_b;

    // allocate reactive list heads
    test->a   = (node_t*)malloc(sizeof(node_t*));
    test->b   = (node_t*)malloc(sizeof(node_t*));
    test->out = (node_t*)malloc(sizeof(node_t*));

    if (test->a == NULL || test->b == NULL || test->out == NULL) 
        return "cannot initialize list heads";

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
        return "can't build sorted input lists";

    

    // output list is initially empty
    test->out = NULL;

    return NULL;
}


// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) {

    list_merge (test->a, test->b, &(test->out));

    return NULL;
}


// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) {

    return NULL;
}


#if CONV || CHECK

// ---------------------------------------------------------------------
// make_conv_input
// ---------------------------------------------------------------------
char* make_conv_input(test_t* test) {

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
        if (!list_is_sorted(test->a))
            return "conventional list a is not sorted";
        if (!list_is_sorted(test->b)) 
            return "conventional list b is not sorted";
    }
    #endif
    
    // output list is initially empty
    test->out = NULL;    

    return NULL;
}


// ---------------------------------------------------------------------
// do_conv_eval
// ---------------------------------------------------------------------
char* do_conv_eval(test_t* test) {
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
