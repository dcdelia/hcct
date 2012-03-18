#include <stdio.h>
#include <stdlib.h>

#include "ceal.h"
#include "modlist.h"

//item generator...
static intptr_t rand_int() 
{
  return rand();
}

//comparator...
intptr_t isle (void* v1, void* v2) 
{
  return ((intptr_t) v1) <= ((intptr_t) v2);
}

// ---------------------------------------------------------------------
//  modlist_new_sorted_rand
// ---------------------------------------------------------------------
modref_t* modlist_new_sorted_rand (int n, int max_delta) 
{
    //creates empty list...
    modref_t* list = modlist_random (0, rand_int);
    
    int curr_val = n*max_delta;
    
    for (; n>0; n--) 
    {
        curr_val -= (1 + rand() % max_delta);
        modlist_insert (list, (void *)curr_val);
    }
    
    return list;
}

/*
#define N 10
#define MAX_DELTA 100

int main ()
{
    slime_t *comp=slime_open ();
    
    //list atom...
    cons_cell_t* c;
    
    //this modrefs are created by list generators...
    modref_t *theRandomSortedList_1, *theRandomSortedList_2;
    
    //merge() does not create this modref, so we have to do it...
    modref_t *theMergedList=modref ();
    
    //generates random sorted list #1...
    theRandomSortedList_1=list_new_sorted_rand (N, MAX_DELTA);
    
    //prints it...
    printf ("Random sorted list #1:\n");
    cons_cell_t* c = modref_deref(theRandomSortedList_1);
    while (c)
    {
        printf ("%d ", c->hd);
        c=modref_deref (c->tl);
    }
    printf ("\n\n");
    
    //generates random sorted list #2...
    theRandomSortedList_2=list_new_sorted_rand (N, MAX_DELTA);
    
    //prints it...
    printf ("Random sorted list #2:\n");
    cons_cell_t* c = modref_deref(theRandomSortedList_2);
    while (c)
    {
        printf ("%d ", c->hd);
        c=modref_deref (c->tl);
    }
    printf ("\n\n");
    
    //merges lists...
    merge(read(theRandomSortedList_1), read(theRandomSortedList_2), isle, theMergedList);
    
    //prints merged list...
    printf ("Merged list:\n");
    cons_cell_t* c = modref_deref(theMergedList);
    while (c)
    {
        printf ("%d ", c->hd);
        c=modref_deref (c->tl);
    }
    printf ("\n\n");
    
    //makes some changes...
    slime_meta_start ();
    modlist_insert (theRandomSortedList_1, (void *)100);
    slime_propagate ();
    //prints merged list...
    printf ("Merged list:\n");
    cons_cell_t* c = modref_deref(theMergedList);
    while (c)
    {
        printf ("%d ", c->hd);
        c=modref_deref (c->tl);
    }
    printf ("\n\n");
    
    slime_close (comp);
    
    return 0;
}
*/
