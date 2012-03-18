/* ============================================================================
 *  LScheduler.c
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        February 25, 2008
 *  Module:         dc

 *  Last changed:   $Date: 2010/11/15 09:54:38 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/

#include <stdio.h>
#include <stdlib.h>

//#include "LQueue.h"
#include "LHeap.h"

#include "LScheduler.h"
#include "_dc.h"

#define ALLOW_SELF_TRIGGER 0

/*global vars...*/
//LQueue *g_Queue;
LHeap *g_Heap;
unsigned g_UserComp=0;

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
        _Comp()
============================*/
/* Default comparator for the Heap */
static Bool _DefComp (ui4 inA, ui4 inB) {
	return inA < inB;
}

/*===========================
        Init()
============================*/
void LScheduler_Init (LScheduler_TComparator inComparator)
{
	if (inComparator==NULL)
	{
		//compares timestamps...
		g_Heap=LHeap_New (_DefComp);
	}
	else
	{
		//user defined comparator...
		g_Heap=LHeap_New (inComparator);
		g_UserComp=1;
	}
        /*creates queue...*/
        //g_Queue=LQueue_New (LType_Ptr);
        
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
                        /*enqueues constraint...*/
                        //printf ("Enqueueing constraint %d\n", theConsID);
                        //LQueue_EnqueuePtr (g_Queue, (void *)theConsID);
			
			if (g_UserComp)
				LHeap_Add (g_Heap, (void *)theConsID, (ui4)LCP_GetConsParam (theConsID));
			else
				LHeap_Add (g_Heap, (void *)theConsID, (ui4)LCP_GetConsTimeStamp (theConsID));
			
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
	ui4 theConsPri;
        
        //printf ("Entering Extract()...\n");
        
        /*if empty heap returns null...*/
	if (LHeap_Empty (g_Heap))
        {
                //printf ("Exiting Extract()...\n");
                return NULL;
        }
                
        /*dequeues constraint...*/
        //theConsID=(long int)LQueue_DequeuePtr (g_Queue);
	LHeap_ExtractMin (g_Heap, &theConsID, &theConsPri);
        /*if it has been deleted...*/
        while (LCP_GetConsInfo (theConsID)==-2)
        {
                /*resets info field...*/
                LCP_SetConsInfo (theConsID, NULL);
                
                /*dequeues another...*/
		if (LHeap_Empty (g_Heap))
                {
                        //printf ("Exiting Extract()...\n");
                        return NULL;
                }
                else
			LHeap_ExtractMin (g_Heap, &theConsID, &theConsPri);
                       //theConsID=(long int)LQueue_DequeuePtr (g_Queue);                        
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
        /*deletes heap...*/
        //LQueue_Delete (&g_Queue);
	LHeap_Delete (&g_Heap);
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
