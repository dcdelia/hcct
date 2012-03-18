/* ============================================================================
 *  LAllocator.c
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        February 7, 2008
 *  Module:         ??

 *  Last changed:   $Date: 2010/09/15 08:42:03 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#include <stdio.h>
#include <stdlib.h>

#include "LAllocator.h"

/*defines...*/

#define LAllocator_BASE_SIZE 16
//#define LAllocator_BASE_SIZE 2000000
#define LAllocator_RESIZE_FACTOR 1.5
#define LAllocator_NUM_CTRL_FIELDS 5

/*l: index of first freed slot...*/
#define l_index 0
/*n: max slots allocated so far...*/
#define n_index 1
/*h: max available slots...*/
#define h_index 2
/*a: currently allocated slots...*/
#define a_index 3
/*s: slot size...*/
#define s_index 4

/*===========================
        Init()
============================*/
void LAllocator_Init (unsigned long record_size, void **ptr)
{
        unsigned long int *theAuxPtr;
        
        if (record_size<8)
        {
        	printf ("[DEBUG] [ERROR] LAllocator: record must be at least 8 bytes long!\n");
        	*ptr=NULL;
        	return;
        }
       
        /*allocates memory...*/
        theAuxPtr=malloc ((LAllocator_BASE_SIZE * record_size) + (LAllocator_NUM_CTRL_FIELDS * sizeof (unsigned long int)));
        
        /*sets l (index of first freed slot)...*/
        theAuxPtr[l_index]=-1;
        
        /*sets n (max slots allocated so far)...*/
        theAuxPtr[n_index]=1;
        
        /*sets h (max available slots)...*/
        theAuxPtr[h_index]=LAllocator_BASE_SIZE;
        
        /*sets a (currently allocated slots)...*/
        theAuxPtr[a_index]=0;
        
        /*sets s (slot size)...*/
        theAuxPtr[s_index]=record_size;
        
        /*returns pointer to free mem...*/
        *ptr=(theAuxPtr+LAllocator_NUM_CTRL_FIELDS);
}

/*===========================
        Alloc()
============================*/
unsigned long int LAllocator_Alloc (void **ptr)
{
        unsigned long int out_index;
        unsigned long int *theAuxPtr=*ptr;
        char *theAuxPtr2=*ptr;
        unsigned long int* theAuxPtr3;
        
        theAuxPtr=theAuxPtr-LAllocator_NUM_CTRL_FIELDS;
        
        /*if freed slot available...*/
        if (theAuxPtr[l_index]!=-1)
        {
                //printf ("Reusing a previously freed block...\n");
                
                out_index=theAuxPtr[l_index];
                
                /*l = l->next */
                theAuxPtr3=(unsigned long int *)&(theAuxPtr2[theAuxPtr[l_index]*theAuxPtr[s_index]]);
                theAuxPtr[l_index]=theAuxPtr3[1];
                //printf ("New l index: %d\n", theAuxPtr[l_index]);
                
                /*to clear the flag bit...*/
                /*is it necessary? No, if the block is used immediately...*/
                theAuxPtr3[0]=0;
        }
        else /*no freed slots available...*/
        {
                /*if n < h...*/
                if (theAuxPtr[n_index]<theAuxPtr[h_index])
                {
                        //printf ("n<h...\n");
                        
                        out_index=theAuxPtr[n_index];
                        theAuxPtr[n_index]=theAuxPtr[n_index]+1; 
                       
                }
                else /*all slots have been allocated...*/
                {
                       //printf ("reallocating memory...");
                        
                        /*reallocs memory...*/
                        theAuxPtr=realloc (theAuxPtr, (theAuxPtr[h_index]*LAllocator_RESIZE_FACTOR*theAuxPtr[s_index])+(LAllocator_NUM_CTRL_FIELDS*sizeof (unsigned long int)));
                        
                        /*updates h...*/
                        theAuxPtr[h_index]=theAuxPtr[h_index]*LAllocator_RESIZE_FACTOR;
                        
                        //printf ("A resize occurred!\n");
                        //printf ("h = %u \t n = %u \t a = %u\n", theAuxPtr[h_index], theAuxPtr[n_index], theAuxPtr[a_index]);
                        
                        /*updates ptr...*/
                        *ptr=theAuxPtr+LAllocator_NUM_CTRL_FIELDS;
                        
                        out_index=theAuxPtr[n_index];
                        theAuxPtr[n_index]=theAuxPtr[n_index]+1; 
                        
                }
        }
        
        /*increments allocated blocks count...*/
        theAuxPtr[a_index]=theAuxPtr[a_index]+1;
        
        if (out_index & (1<<31))
        {
                printf ("LAllocator: Out of slot space!!!\n");
                exit (1);
        }
        
        /*returns index...*/
        return out_index;
}

/*===========================
        Free()
============================*/
void LAllocator_Free (unsigned long int index, void **ptr)
{
        unsigned long int *theAuxPtr=*ptr;
        char *theAuxPtr2=*ptr;
        unsigned long int* theAuxPtr3;
        
        theAuxPtr=theAuxPtr-LAllocator_NUM_CTRL_FIELDS;
        
        theAuxPtr3=(unsigned long int *)&(theAuxPtr2[index*theAuxPtr[s_index]]);
        
        /*sets first longword of freed block to -1...*/
        theAuxPtr3[0]=-1;
        /*sets second word to next index...*/
        theAuxPtr3[1]=theAuxPtr[l_index];
        /*sets l to freed block index...*/
        theAuxPtr[l_index]=index;
        
        /*decrements allocated blocks count...*/
        theAuxPtr[a_index]=theAuxPtr[a_index]-1;
}

/*===========================
        CleanUp()
============================*/
void LAllocator_CleanUp (void **ptr)
{
        unsigned long int *theAuxPtr=*ptr;
        
        /*gets block address...*/
        theAuxPtr=theAuxPtr-LAllocator_NUM_CTRL_FIELDS;
        
        /*frees memory...*/
        free (theAuxPtr);
        
        /*sets ptr to null...*/
        *ptr=NULL;
}

/*===========================
        GetN()
============================*/
unsigned long int LAllocator_GetN (void **ptr)
{
        unsigned long int *theAuxPtr=*ptr;
        
        theAuxPtr=theAuxPtr-LAllocator_NUM_CTRL_FIELDS;
        
        return theAuxPtr[n_index];
}

/*===========================
        GetH()
============================*/
unsigned long int LAllocator_GetH (void **ptr)
{
        unsigned long int *theAuxPtr=*ptr;
        
        theAuxPtr=theAuxPtr-LAllocator_NUM_CTRL_FIELDS;
        
        return theAuxPtr[h_index];
}


/* Copyright (C) 2008 Camil Demetrescu, Andrea Ribichini

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
