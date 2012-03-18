/* ============================================================================
 *  add-test-conv.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu and Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/13 16:47:10 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.1 $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "list.h"

#include "test-driver.h"

// specific test object
struct test_s 
{
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    const char* input_family;

    node_t*     a;
    int         sum;
};


// ---------------------------------------------------------------------
// make_test
// ---------------------------------------------------------------------
test_t* make_test(size_t input_size, int seed, 
                 const char* input_family, int check_correctness) 
{
    test_t* test = (test_t*)malloc(sizeof(test_t));
    if (test == NULL) return NULL;

    test->input_size = input_size;
    test->seed = seed;
    test->num_updates = 0;
    test->input_family = input_family;
    
    return test;
}


// ---------------------------------------------------------------------
// del_test
// ---------------------------------------------------------------------
void del_test(test_t* test) 
{   
    list_remove_all (&test->a);
    
    free(test);
}


// ---------------------------------------------------------------------
// get_num_updates
// ---------------------------------------------------------------------
size_t get_num_updates(test_t* test) 
{
    return test->num_updates;
}


// ---------------------------------------------------------------------
// get_test_name
// ---------------------------------------------------------------------
char* get_test_name() 
{
    return "add-conv";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) 
{    
    // initialize random generator
    srand(test->seed);

    // creates random list...
    test->a=list_new_rand (test->input_size, INT_MAX);

    return NULL;
}

#define VERBOSE 0

// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) 
{
    //maps list...
    test->sum=list_sum (test->a);
    
#if VERBOSE

    list_print (test->a);
    printf ("sum: %d\n", test->sum);

#endif
    
    return NULL;
}

// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) 
{
    return NULL;
}


/* Copyright (C) 2010 Camil Demetrescu and Andrea Ribichini

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
