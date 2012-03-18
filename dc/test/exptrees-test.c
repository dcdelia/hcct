/* ============================================================================
 *  exptrees-test.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 13:50:13 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.3 $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dc.h"
#include "exptrees.h"
#include "test-driver.h"


// specific test object
struct test_s {
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    int         check;
    const char* input_family; //not used in this test...
    
    #if CONV || CHECK
    
    #endif

    node_t*     rTreeRoot;
    T           rOutValue;
    size_t      num_leaves;
    node_t**    leafPtrArray;
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
    
    #endif

    return test;
}


// ---------------------------------------------------------------------
// del_test
// ---------------------------------------------------------------------
void del_test(test_t* test) {

    #if CONV || CHECK
    
    #endif

    //deletes leaf ptr array...
    free (test->leafPtrArray);
    
    //deletes tree & associated constraints...    
    //TODO...
    
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
    return "exptr-dc";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) {

    // initialize random generator
    srand(test->seed);

    // creates tree...
    test->rTreeRoot = exptrees_random (test->input_size);
    
    test->rOutValue=(test->rTreeRoot)->num;
    
    // counts leaves...
    test->num_leaves=exptrees_count_leaves (test->rTreeRoot);
    
    // allocates mem for leaf pointer array...
    test->leafPtrArray=(node_t **)malloc (test->num_leaves*sizeof (node_t *));
    
    // fills leaf pointer array...
    exptrees_fill_leaf_ptr_array_ext (test->rTreeRoot, test->leafPtrArray);

    return NULL;
}

// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) {
    
    //nothing to do (tree is evaluated as it is built...)
    
    return NULL;
}


// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) 
{
    int i;
    T theBackup;
    
    for (i=0; i<test->num_leaves; i++) 
    {
            theBackup = (test->leafPtrArray[i])->num;

            begin_at ();
            (test->leafPtrArray[i])->num=100;
            end_at ();
            
            begin_at ();
            (test->leafPtrArray[i])->num=theBackup;
            end_at ();
            
            test->num_updates=test->num_updates+2;
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

    T theRes=exptrees_conv_eval (test->rTreeRoot);
    
    if (theRes!=test->rOutValue)
        return "error in tree evaluation";

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
