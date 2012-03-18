/* ============================================================================
 *  LScheduler.c
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        February 25, 2008
 *  Module:         dc

 *  Last changed:   $Date: 2010/11/15 09:54:36 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/

#include <stdio.h>
#include <stdlib.h>

#include "pool.h"
#include "LScheduler.h"
#include "_dc.h"

#define ALLOW_SELF_TRIGGER 0

//page size for list pool...
#define MEM_POOL_PAGE_SIZE   1000

//list atom...
typedef struct tagLScheduler_node_t
{
    long int mConsID;
    struct tagLScheduler_node_t *mNext;
} LScheduler_node_t;

/*global vars...*/

// pointer to list...
LScheduler_node_t *g_List;
// list pool & list pool free list...
pool_t *g_ListPool;
void *g_ListPoolFreeList;

/*===========================
        __deleteConsCallback()
============================*/
void __deleteConsCallback (unsigned long index)
{
    /*if handler scheduled...*/
    if (LCP_GetConsInfo (index)==-1)
        LCP_SetConsInfo (index, (void *)-2); /*sets status to 'deleted'...*/
    else
        LCP_SetConsInfo (index, NULL); /*resets info field...*/
}

/*===========================
        Init()
============================*/
void LScheduler_Init (LScheduler_TComparator inComparator)
{
    //initializes memory pool...
    g_ListPool=pool_init (MEM_POOL_PAGE_SIZE, sizeof (LScheduler_node_t), &g_ListPoolFreeList);
        
    /*installs deletecons callback...*/
    LCP_InstallDeleteConsCallback (__deleteConsCallback);
}

/*===========================
        Touched()
============================*/
void LScheduler_Touched (unsigned long cell_id, long int g_CurrCons)
{
    long int theConsID;
    void *theIterator;
    
    LScheduler_node_t *theNewAtom;
        
    //printf ("Entering Touched()...\n");
        
    /*scans outgoing arcs and enqueues constraints...*/
    theIterator=LCP_GetFirstArc (cell_id, &theConsID);
    while (theIterator!=NULL)
    {
        /*if constraint not already scheduled and not deleted AND not self...*/
        #if ALLOW_SELF_TRIGGER
        if (LCP_GetConsInfo (theConsID)!=-1 && LCP_GetConsInfo (theConsID)!=-2)
        #else
        if (LCP_GetConsInfo (theConsID)!=-1 && LCP_GetConsInfo (theConsID)!=-2 && (theConsID!=g_CurrCons))
        #endif
        {
            //allocates mem for new list atom...
            pool_alloc (g_ListPool, g_ListPoolFreeList, theNewAtom, LScheduler_node_t);
            
            //inserts constraint into list...
            theNewAtom->mConsID=theConsID;
            theNewAtom->mNext=g_List;
            g_List=theNewAtom;
			
            /*sets 'enqueued' flag...*/
            LCP_SetConsInfo (theConsID, (void *)-1);
        }
                
            /*gets next arc...*/
            theIterator=LCP_GetNextArc (theIterator, &theConsID);
            //printf ("Touched: Iterator=%u\n", theIterator);
    }
        
    //printf ("Exiting Touched()...\n");
}

/*===========================
        Extract()
============================*/
unsigned long LScheduler_Extract ()
{
    long int theConsID;
    
    LScheduler_node_t *theAuxAtom;
        
    //printf ("Entering Extract()...\n");
        
    /*if empty list returns null...*/
	if (g_List==NULL)
    {
        //printf ("Exiting Extract()...\n");
        return NULL;
    }
    
    //extracts constraint from list...
    theAuxAtom=g_List;
    g_List=g_List->mNext;    
    theConsID=theAuxAtom->mConsID;
    //deallocates atom...
    pool_free (theAuxAtom, g_ListPoolFreeList);
    
    
    while (LCP_GetConsInfo (theConsID)==-2)
    {
        /*resets info field...*/
        LCP_SetConsInfo (theConsID, NULL);
                
        /*extracts another...*/
		if (g_List==NULL)
        {
            //printf ("Exiting Extract()...\n");
            return NULL;
        }
        else
        {
			theAuxAtom=g_List;
            g_List=g_List->mNext;    
            theConsID=theAuxAtom->mConsID;
            //deallocates atom...
            pool_free (theAuxAtom, g_ListPoolFreeList); 
        }
    }
                
    /*resets info field...*/
    LCP_SetConsInfo (theConsID, NULL);
        
    //printf ("Exiting Extract()...\n");
        
    /*returns cons id...*/
    return (unsigned long)theConsID;
}

/*===========================
        CleanUp()
============================*/
void LScheduler_CleanUp ()
{
    /*deletes pool...*/
    pool_cleanup (g_ListPool);
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
