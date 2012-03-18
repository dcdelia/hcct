/* ============================================================================
 *  exptrees-test-ceal.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu and Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 13:50:14 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.4 $
*/


#include <stdio.h>
#include <string.h>

#include "ceal.h"
//#include "modlist.h"
#include "exptrees.h"

#include "test-driver.h"

// specific test object
struct test_s 
{
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    const char* input_family;

    modref_t*   rTreeRoot;
    modref_t*   rOutValue;
    size_t      num_leaves;
    modref_t**  leafPtrArray;
    
    slime_t*    comp;
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
    
    //initializes ceal...
    test->comp=slime_open ();
    
    return test;
}


// ---------------------------------------------------------------------
// del_test
// ---------------------------------------------------------------------
void del_test(test_t* test) 
{   
    //ceal clean up...
    slime_close (test->comp);
    
    //deletes leaf ptr array...
    free (test->leafPtrArray);
    
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
    return "exptr-ceal";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) 
{    
    // initialize random generator
    srand(test->seed);

    // creates tree...
    test->rTreeRoot=exptrees_random (test->input_size);

    //if it's here, it DOESN'T WORK!!!
    //creates modref for output tree value...
    //test->rOutValue=modref ();
    
    //counts leaves...
    test->num_leaves=exptrees_count_leaves (test->rTreeRoot);
    
    //allocates mem for leaf pointer array...
    test->leafPtrArray=(node_t **)malloc (test->num_leaves*sizeof (modref_t *));
  
    //fills leaf pointer array...
    exptrees_fill_leaf_ptr_array_ext (test->rTreeRoot, test->leafPtrArray);

    return NULL;
}


// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) 
{
    //creates modref for output tree value...
    //it MUST be here...
    test->rOutValue=modref ();
    //evaluates tree...
    //scope ();
    exptrees_eval (test->rTreeRoot, test->rOutValue);
    
    return NULL;
}

// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) 
{
    int i;
    leaf_t *theBackup, *theModLeaf=exptrees_leaf (100);
    
    test->num_updates = 0;

    //printf ("Initial tree value: %d\n", modref_deref (test->rOutValue));
    
    for (i=0; i<test->num_leaves; i++) 
    {
        //replaces leaf...
        slime_meta_start ();    
        theBackup=modref_deref (test->leafPtrArray[i]); 
        write (test->leafPtrArray[i], theModLeaf);
        slime_propagate ();
    
        //printf ("Tree value: %d\n", modref_deref (test->rOutValue));
        
        //puts back original leaf...
        slime_meta_start ();
        write (test->leafPtrArray[i], theBackup);
        slime_propagate ();
        
        //printf ("Tree value: %d\n", modref_deref (test->rOutValue));
        
        test->num_updates=test->num_updates+2;
    }

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
