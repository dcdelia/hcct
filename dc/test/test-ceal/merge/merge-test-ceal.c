/* ============================================================================
 *  merge-test-ceal.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu and Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 13:50:16 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.9 $
*/


#include <stdio.h>
#include <string.h>

#include "ceal.h"
#include "modlist.h"
#include "merge.h"

#include "test-driver.h"


// maximum difference between consecutive values in the input list
#define MAX_DELTA   20


// size of a = input_size - input_size*SPLIT_RATIO
// size of b = input_size*SPLIT_RATIO
#define SPLIT(x,y)      ((double)(x)/((x)+(y)))
#define SPLIT_A(n,x,y)  ((size_t)((n)*SPLIT(x,y)))
#define SPLIT_B(n,x,y)  (((n)-(size_t)((n)*SPLIT(x,y))))


// specific test object
struct test_s 
{
    size_t      input_size;
    int         seed;
    size_t      num_updates;
    const char* input_family;

    modref_t*   ra;
    modref_t*   rb;
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
    return "merger-ceal";
}


// ---------------------------------------------------------------------
// make_updatable_input
// ---------------------------------------------------------------------
char* make_updatable_input(test_t* test) 
{    

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
    test->ra = modlist_new_sorted_rand (SPLIT_A(test->input_size, side_a, side_b), MAX_DELTA);
    test->rb = modlist_new_sorted_rand (SPLIT_B(test->input_size, side_a, side_b), MAX_DELTA);

    //creates modref for output list...
    test->rout=modref ();

    return NULL;
}


// ---------------------------------------------------------------------
// do_from_scratch_eval
// ---------------------------------------------------------------------
char* do_from_scratch_eval(test_t* test) 
{
    //merges lists...
    scope ();
    merge (read (test->ra), read (test->rb), isle, test->rout);
    
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
    //list atom...
    cons_cell_t* c;
    
    //backup list ptr...
    modref_t *thePtr;
    
    test->num_updates = 0;

#if VERBOSE
    printf ("List a: ");
    _dump_list (test->ra);
    printf ("List b: ");
    _dump_list (test->rb);
    printf ("List out: ");
    _dump_list (test->rout);
    printf ("\n");
#endif
    
    //rem_ins loop for list a...
    thePtr=test->ra;
#if VERBOSE
    printf ("Entering loop on list a...\n");
#endif
    while (modref_deref (test->ra)!=NULL)
    {
        void *theTmp;
        
        //removes item...
        slime_meta_start ();
        theTmp=modlist_remove (test->ra);
        slime_propagate ();
        
#if VERBOSE
        printf ("Removed item!\n");
        printf ("List a: ");
        _dump_list (thePtr);
        printf ("List b: ");
        _dump_list (test->rb);
        printf ("List out: ");
        _dump_list (test->rout);
        printf ("\n");
#endif

        //puts item back...
        slime_meta_start ();
        modlist_insert (test->ra, theTmp);
        slime_propagate ();
        
#if VERBOSE
        printf ("Put item back!\n");
        printf ("List a: ");
        _dump_list (thePtr);
        printf ("List b: ");
        _dump_list (test->rb);
        printf ("List out: ");
        _dump_list (test->rout);
        printf ("\n");
#endif
        
        c = modref_deref (test->ra);
        test->ra=c->tl;
        
        test->num_updates+=2;
    }    
    test->ra=thePtr;
    
    //rem_ins loop for list b...
    thePtr=test->rb;
#if VERBOSE
    printf ("\nEntering loop on list b...\n");
#endif
    while (modref_deref (test->rb)!=NULL)
    {
        void *theTmp;
        
        //removes item...
        slime_meta_start ();
        theTmp=modlist_remove (test->rb);
        slime_propagate ();

#if VERBOSE
        printf ("Removed item!\n");
        printf ("List a: ");
        _dump_list (test->ra);
        printf ("List b: ");
        _dump_list (thePtr);
        printf ("List out: ");
        _dump_list (test->rout);
        printf ("\n");
#endif

        //puts item back...
        slime_meta_start ();
        modlist_insert (test->rb, theTmp);
        slime_propagate ();

#if VERBOSE
        printf ("Put item back!\n");
        printf ("List a: ");
        _dump_list (test->ra);
        printf ("List b: ");
        _dump_list (thePtr);
        printf ("List out: ");
        _dump_list (test->rout);
        printf ("\n");
#endif

        c = modref_deref (test->rb);
        test->rb=c->tl;
        
        test->num_updates+=2;
    }    
    test->rb=thePtr;

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
