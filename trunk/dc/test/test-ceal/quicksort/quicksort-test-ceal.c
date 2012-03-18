/* ============================================================================
 *  mergesort-test-ceal.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu and Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/12 10:47:43 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.1 $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ceal.h"
#include "modlist.h"
#include "quicksort.h"
#include "_quicksort.h"

#include "test-driver.h"

void _dump_list (modref_t *inList);

// specific test object
struct test_s 
{
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    const char* input_family;

    modref_t*   ra;
    modref_t*   rout;
    
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
    return "quicksort-ceal";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) 
{    
    // initialize random generator
    srand(test->seed);

    // creates random list...
    test->ra=modlist_random (test->input_size, rand_int);
    //_dump_list (test->ra);

    return NULL;
}


// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) 
{    
    //creates modref for output list...
    test->rout=modref ();
  
    scope ();
    quicksort (test->ra, isgt, test->rout);
    
    return NULL;
}


void _dump_list (modref_t *inList)
{
    //list atom...
    cons_cell_t* c;
    
    c = modref_deref(inList);
    while (c)
    {
        printf ("%d ", c->hd);
        c=modref_deref (c->tl);
    }
    printf ("\n");
}


#define VERBOSE 0

// ---------------------------------------------------------------------
// do_updates
// ---------------------------------------------------------------------
char* do_updates(test_t* test) 
{
    int i, k=0;
    
    //list atom...
    cons_cell_t* c;
    
    //backup list ptr...
    modref_t *thePtr;
    
    test->num_updates = 0;
    
    if (!strcmp (test->input_family, "k=1"))
        k=1;
    if (!strcmp (test->input_family, "k=2"))
        k=2;
    if (!strcmp (test->input_family, "k=4"))
        k=4;
    if (!strcmp (test->input_family, "k=8"))
        k=8;

#if VERBOSE
    printf ("List a: ");
    _dump_list (test->ra);
    printf ("List out: ");
    _dump_list (test->rout);
    printf ("\n");
#endif

    if (k==0)
    {
        //ins_rem loop for list a...
        thePtr=test->ra;
#if VERBOSE
        printf ("Entering loop on list a...\n");
#endif
        while (modref_deref (test->ra)!=NULL)
        {
            //inserts item...
            slime_meta_start ();
        
            modlist_insert (test->ra, rand ());
        
            slime_propagate ();
    
#if VERBOSE
            printf ("Inserted item!\n");
            printf ("List a: ");
            _dump_list (thePtr);
            printf ("List out: ");
            _dump_list (test->rout);
            printf ("\n");
#endif
        
            //removes item...
            slime_meta_start ();
        
            modlist_remove (test->ra);
        
            slime_propagate ();
        
#if VERBOSE
            printf ("Removed item!\n");
            printf ("List a: ");
            _dump_list (thePtr);
            printf ("List out: ");
            _dump_list (test->rout);
            printf ("\n");
#endif
        
            c = modref_deref (test->ra);
            test->ra=c->tl;
        
            test->num_updates+=2;
        }    
        test->ra=thePtr;
    }

    if (k!=0)
    {    
        //ins_rem loop for list a...
        thePtr=test->ra;
#if VERBOSE
        printf ("Entering loop on list a...\n");
#endif
        while (modref_deref (test->ra)!=NULL)
        {
            //inserts item(s)...
            slime_meta_start ();
        
            for (i=0; i<k; i++)
                modlist_insert (test->ra, rand ());
        
            slime_propagate ();
    
#if VERBOSE
            printf ("Inserted item(s)!\n");
            printf ("List a: ");
            _dump_list (thePtr);
            printf ("List out: ");
            _dump_list (test->rout);
            printf ("\n");
#endif
        
            //removes item(s)...
            slime_meta_start ();
        
            for (i=0; i<k; i++)
                modlist_remove (test->ra);
        
            slime_propagate ();
        
#if VERBOSE
            printf ("Removed item(s)!\n");
            printf ("List a: ");
            _dump_list (thePtr);
            printf ("List out: ");
            _dump_list (test->rout);
            printf ("\n");
#endif
        
            c = modref_deref (test->ra);
            test->ra=c->tl;
        
            test->num_updates+=2;
        }    
        test->ra=thePtr;
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
