/* ============================================================================
 *  dc.c
 * ============================================================================

 *  Author:         (c) 2008-2010 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        January 24, 2008
 *  Module:         dc

 *  Last changed:   $Date: 2010/11/15 12:56:15 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.40 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "dc.h"
#include "_dc.h"
#include "pool.h"
#include "LScheduler.h"


// defines...

// 0 means inline patching is off
// 1 means inline patching is on
// 2 means inline patching is simulated in C (for debugging purposes)
#define DC_INLINE_PATCH 0

// don't set max to 0xffffffff, since we've used -1, -2,...as special values...
// could be extended to 64 bits if needed...
#define LCP_MaxTS 0xfffffff0

// page size for all memory pools...
#define MEM_POOL_PAGE_SIZE   1000

// low-level event counters
dc_stat_t dc_stat;


/*typedefs...*/

typedef cons_t LCP_THandler;

typedef struct tagArc 
{
    struct tagArc *mNext; /*next list item*/   
    
    int mConsID; /*constraint id*/
    unsigned long mTS; /*arc timestamp*/
    
} LCP_TArc;


typedef struct tagCell 
{
    unsigned long mCTSR; /*current timestamp - read*/
    unsigned long mCTSW; /*current timestamp - write*/
    
    LCP_TArc *mHandlerList; /*handler list*/
    	
    unsigned long mData; /*data stored in mem cell*/
	
    long mCountdown; /*countdown to handler list refresh*/
    
} LCP_TCell;


typedef struct tagWrCells 
{
    struct tagWrCells *mNext; /*next list item...*/
        
    unsigned long mOldValue; /*cell's old value*/
        
    unsigned long mCell; /*cell reference*/
        
} LCP_TWrCells;


typedef struct tagExecutedCons 
{
    struct tagExecutedCons *mNext;    
    int mConsIndex; //cons id really...
	
} LCP_TExecutedCons;


typedef struct tagCons 
{ 
    unsigned long mHTS; /*handler timestamp*/
    
    LCP_THandler mHandler; /*handler pointer*/
    void *mParam; /*handler param*/
        
    LCP_THandler mFinalHandler; /*final handler pointer*/
    unsigned long mFinalScheduled; /*huge waste of space...signals whether handler has been executed in current constraint evaluation session...*/
        
    unsigned long mRefCount; /*# of arcs that reference the constraint*/
    void *mConsInfo;
    
} LCP_TCons;


// Etichette per il codice delle patch inline
extern char handler_call_start_RA;
extern char handler_call_end_RA;
extern char handler_call_RA;
//extern char handler_timestamp_k_RA;
//extern char handler_timestamp_factor_RA;
//extern char handler_timestamp_base_RA;
extern char handler_timestamp_gcellptr_RA;
extern char handler_timestamp_maints_RA;

extern char handler_call_start_RC;
extern char handler_call_end_RC;
extern char handler_call_RC;
//extern char handler_timestamp_k_RC;
//extern char handler_timestamp_factor_RC;
//extern char handler_timestamp_base_RC;
extern char handler_timestamp_gcellptr_RC;
extern char handler_timestamp_maints_RC;

extern char handler_call_start_WA;
extern char handler_call_end_WA;
extern char handler_call_WA;

extern char handler_call_start_WC;
extern char handler_call_end_WC;
extern char handler_call_WC;

extern char handler_call_start_RWA;
extern char handler_call_end_RWA;
extern char handler_call_read_RWA;
extern char handler_call_write_RWA;

extern char handler_call_start_RWC;
extern char handler_call_end_RWC;
extern char handler_call_read_RWC;
extern char handler_call_write_RWC;

// global variables...

// memory pools & free lists...
pool_t *g_ArcPool;
void *g_ArcPoolFreeList;
pool_t *g_WrCellsPool;
void *g_WrCellsPoolFreeList;
pool_t *g_ConsPool;
void *g_ConsPoolFreeList;
pool_t *g_ExConsPool;
void *g_ExConsPoolFreeList;

// timestamp generator...
//don't initialize with 0, which has special meaning...
unsigned long g_MainTS=1;

// counter of number of executed constraints
unsigned long g_num_exec_cons = 0;

// current number of constraints
unsigned long g_ConsCount=0;

/*mode flag...*/
/* 0 = 'main' mode */
/* 1 = 'handler execution' mode */
/* 2 = 'atomic sequence' mode */
/* 3 = 'final handler execution' mode */
unsigned long g_mode=0; 

// atomic seq's counter...
unsigned long g_AtomicCount=0;

// id of current constraint (g_mode==1)...
int g_CurrCons=NULL;

//set by inline patch...
unsigned long g_CurrCell=NULL;

// pre-write & post-write values...
unsigned long g_prewrite_longword;
unsigned long g_postwrite_longword;

// required by newcons()...
unsigned long g_DummyCellIndex;
unsigned long *g_DummyPtr;

// callback function pointers...
LCP_TCallback g_NewConsCallback=NULL;
LCP_TCallback g_DeleteConsCallback=NULL;

// written cells list
LCP_TWrCells *g_WrCList=NULL;

// executed handlers list
LCP_TExecutedCons *g_ExecutedConsList=NULL;

// flag that tells if dc was initialized
int g_initialized;


/*======================================
        __post_write_lcp_handler()
=======================================*/
void __post_write_lcp_handler (void *faulty_addr, unsigned long access_size)
{       
    unsigned long theCellIndex;
        
    LCP_TCell *theAuxCell;
    unsigned long theAuxCellIndex;
                
    LCP_TCell *theCell;
    LCP_TArc *theArc;
    LCP_TCons *theCons;
        
    LCP_TWrCells *theWrCell;
        
    LCP_TWrCells *theAuxWrCell;
        
    //cons index === cons id...
    int theConsIndex;
	
    //printf ("[POST-WRITE] Entering with mode: %u\n", g_mode);

//********************************************************
//
// IMPORTED FROM PRE-WRITE...
//
//********************************************************

    LCP_TCons *theCurrCons;
        
    unsigned long theAuxTimeStamp;
	
    theCellIndex=faulty_addr;
    theCell=rm_get_shadow_rec_fast (theCellIndex, long, LCP_TCell);
    g_prewrite_longword=theCell->mData;
    
    //printf ("[POST-WRITE] Entering with mode: %u\n", g_mode);

    #if DC_STAT == 1
    ++dc_stat.mem_write_count;
    #endif

/*if in 'handler execution' mode or in 'atomic sequence' mode or 'final handler execution' mode...*/
    if (g_mode==1 || g_mode==2 || g_mode==3)
    {	  
        if (g_CurrCons!=NULL)
        {
            theCurrCons=(LCP_TCons *)g_CurrCons;
            theAuxTimeStamp=theCurrCons->mHTS;
        }
        else
            theAuxTimeStamp=g_MainTS;
                        
        /*if not already written...*/
        if (theCell->mCTSW < theAuxTimeStamp)
        {   
            #if DC_STAT == 1
            ++dc_stat.first_mem_write_count;
            #endif

            //printf ("[POST-WRITE] Creating written cell item...\n");
            /*updates timestamp...*/
            theCell->mCTSW=theAuxTimeStamp;
                                
            /*creates new written cell item...*/
            pool_alloc (g_WrCellsPool, g_WrCellsPoolFreeList, theWrCell, LCP_TWrCells);
            //printf ("Created WrCell list item!\n");
                                
            theWrCell->mOldValue=theCell->mData;                                
            theWrCell->mCell=theCellIndex;
                                
            /*inserts it as head of written cells list...*/
            theWrCell->mNext=g_WrCList;
            g_WrCList=theWrCell;
        }                       
    }
    
    //printf ("[POST-WRITE] Updating cell data...\n");
    theCell->mData=*(unsigned long *)rm_get_inactive_ptr (theCellIndex);
    //printf ("[POST-WRITE] Updated cell data...\n");

//********************************************************
//
// END OF IMPORT FROM PRE-WRITE...
//
//********************************************************
	
    /*if in main mode...*/
    if (g_mode==0)
    {
        g_postwrite_longword=theCell->mData;
             
        /*if actual write...*/
        if (g_postwrite_longword!=g_prewrite_longword)
        {
            //printf ("[POST WRITE] actual write\n");
            LScheduler_Touched (theCellIndex, g_CurrCons);

            /*switches to 'handler execution' mode...*/
            g_mode=1;
solve_loop:                                
            theConsIndex=LScheduler_Extract ();
            //printf ("[POST WRITE] entering solve loop...\n");
            while (theConsIndex!=NULL || g_WrCList!=NULL)
            {                               
                /*executes handlers...*/
                /*updates global timestamp...*/
                g_MainTS++;
                                        
                //timestamp reset...
                if (g_MainTS > LCP_MaxTS)
                {
                    //we are not resetting timestamps for the time being...
                    printf ("[ABORT] Run out of timestamps!!!\n");
                    exit (1);  						
                }
                                        
                if (theConsIndex!=NULL)
                {
                    theCons=(LCP_TCons *)theConsIndex;
                                        
                    /*if constraint has been deleted...*/
                    /*should happen only if a constraint is deleted from within another constraint...*/
                     if (theCons->mHTS==-2)
                     { 
                         /*if ref count is 0...*/
                         if (theCons->mRefCount==0)
                         {
                             /*deallocates constraint...*/
                             pool_free (theCons, g_ConsPoolFreeList);
                             //printf ("Deallocated ConsID %d\n", theConsIndex);
                             /*continues...*/
                             theConsIndex=LScheduler_Extract ();// added August 1st 2008 (should correct a bug...)
                             continue;
                         }
                         else
                         {	
                             /*skips it...*/
                             theConsIndex=LScheduler_Extract ();// added August 1st 2008 (should correct a bug...)
                             continue;       
                         }
                    }
                    
                    /*updates handler timestamp...*/
                    theCons->mHTS=g_MainTS;
                    //printf ("Set new handler timestamp...\n");
                                        
                    /*sets global pointer to constraint...*/
                    g_CurrCons=(int)theConsIndex;
                    //printf ("Set global cons pointer...\n");
                                        
                    /*executes handler...*/
                    //printf ("[POST-WRITE] Executing handler at %p with param %p\n", theCons->mHandler, theCons->mParam);
                    theCons->mHandler (theCons->mParam);
                    //printf ("[POST-WRITE] Executed handler...\n");

                    // count number of constraint executions
                    g_num_exec_cons++;
                }
                                        
                /*processes written cells list...*/
                while (g_WrCList!=NULL)
                {
                    //printf ("[POST-WRITE] Processing written cell list...\n");
                                                
                    /*gets current item...*/
                    theWrCell=(LCP_TWrCells *)g_WrCList;
                                                
                    theAuxCellIndex=theWrCell->mCell;
                    theAuxCell=rm_get_shadow_rec_fast (theAuxCellIndex, long, LCP_TCell);
                                                
                    /*if value changed...*/
                    if (theWrCell->mOldValue!=theAuxCell->mData)
                    {
                        //printf ("Value changed!\n");
                        LScheduler_Touched (theAuxCellIndex, g_CurrCons);
                    }
                                                
                    /*removes list item...*/
                    theAuxWrCell=(LCP_TWrCells *)g_WrCList;
                    g_WrCList=theAuxWrCell->mNext;
                    pool_free (theWrCell, g_WrCellsPoolFreeList);                                          
                }
                                        
                theConsIndex=LScheduler_Extract ();
            }
                                
            /*if there are final handlers to execute...*/
            if (g_ExecutedConsList!=NULL)
            {
                /*switches to 'final handler execution' mode...*/
                g_mode=3;

                /*walks the list...*/
                while (g_ExecutedConsList!=NULL)
                {
                    LCP_TExecutedCons *theAuxExItem=g_ExecutedConsList;
                    
                    theConsIndex=g_ExecutedConsList->mConsIndex;
                    theCons=(LCP_TCons *)theConsIndex;

                    // added CD101101: if constraint not deleted
                    if (theCons->mHTS != -2) {
                        //this if added by ribbi on Nov 2nd 2010...
                        if (theCons->mFinalHandler!=NULL)
                        {
                            g_CurrCons=theConsIndex;
                            //printf ("[DEBUG]: executing final handler...\n");
                            //printf ("[DEBUG]: theConsIndex = %u\n", theConsIndex);
                            theCons->mFinalHandler (theCons->mParam);
                        }
                        theCons->mFinalScheduled=0;
                    }
                    else {
                        if (theCons->mRefCount==0)
                        {
                            /*deallocates constraint...*/
                            pool_free (theCons, g_ConsPoolFreeList);
                            //printf ("Deallocated ConsID %d\n", inConsID);
                        }
                    }

                    g_ExecutedConsList=g_ExecutedConsList->mNext;
                    
                    pool_free (theAuxExItem, g_ExConsPoolFreeList);
                }
                                	
                /*switches back to 'handler execution' mode...*/
                g_mode=1;
                                	
                /*bad idea?*/
                goto solve_loop;                               	
            }
                                
            /*switches back to 'main' mode...*/
            g_CurrCons=NULL;
            g_mode=0;              
        }
    }        
    //printf ("[POST-WRITE] Exiting...\n");
}

/*======================================
        __post_read_lcp_handler()
=======================================*/
void __post_read_lcp_handler (void *faulty_addr, unsigned long access_size)
{
    //unsigned long theCellIndex;

    LCP_TCell *theNewCell, *theCell;
    LCP_TArc *theNewArc, *theArc, *thePrevArc;

    LCP_TCons *theCurrCons;

    LCP_TCons *theArcCons;
        
    //printf ("[POST-READ] Entering with mode: %u\n", g_mode);
    //fflush (stdout);
        	
    /*if reading from a protected cell in 'main' mode or 'atomic sequence' mode or 'final handler execution' mode...*/
    //if (g_mode==0 || g_mode==2 || g_mode==3)
    //{
        //printf ("[POST-READ HANDLER] Exiting...\n");
        /*do nothing...*/
        //return;
    //}

    #if DC_STAT == 1
    ++dc_stat.mem_read_count;
    #endif

    /*if reading from a protected cell in 'handler execution' mode...*/
    #if DC_INLINE_PATCH == 0
    if (g_mode==1)
    #endif
    /*logs dependency...*/
    { 
        /*if (g_CurrCons==NULL)
        {      
            printf ("[POST-READ] Error: g_CurrCons == NULL\n");
            exit (1);
        }*/      
            
        theCurrCons=(LCP_TCons *)g_CurrCons;
		
        //theCellIndex=faulty_addr;
        //theCell=rm_get_shadow_rec (theCellIndex);
        #if DC_INLINE_PATCH == 0
        theCell=rm_get_shadow_rec_fast (faulty_addr, long, LCP_TCell);
        #endif
        
        #if DC_INLINE_PATCH == 1
        //set by inline patch...
        theCell=g_CurrCell;
        #endif
                        
        /*if old read timestamp...*/
        #if DC_INLINE_PATCH == 0
        if (theCell->mCTSR < g_MainTS)
        #endif
        {
            /*if (theCell->mCTSR >= theCurrCons->mHTS)
            {
                printf ("This should not happen!\n");
                exit (1);
            }*/
            
            /*adds dependency as first item in handler list...*/
            //printf ("[POST-READ] logging dependency...\n");

            #if DC_STAT == 1
            ++dc_stat.first_mem_read_count;
            #endif

            /*creates new arc item...*/
            pool_alloc (g_ArcPool, g_ArcPoolFreeList, theNewArc, LCP_TArc);
                        
            theNewArc->mConsID=g_CurrCons;
            theNewArc->mTS=theCurrCons->mHTS;
                                
            /*inserts item as head of handler list...*/
            theNewArc->mNext=theCell->mHandlerList;
            theCell->mHandlerList=theNewArc;
                                
            /*updates read timestamp...*/
            theCell->mCTSR=theCurrCons->mHTS;
                                
            /*increments reference count...*/
            theCurrCons->mRefCount=theCurrCons->mRefCount+1;
            //printf ("ConsID: %d \t RefCount:%u\n", g_CurrCons, theCurrCons->mRefCount);
                                
            /*decrements countdown...*/
            if (theCell->mCountdown>-1) 
                theCell->mCountdown--;
            
            /*if reached -1...*/
            if (theCell->mCountdown==-1)
            {
                /*cleans list...*/
                                                       
                thePrevArc=NULL;
                theArc=theCell->mHandlerList;

                while (theArc!=NULL)
                { 
                    theArcCons=(LCP_TCons *)theArc->mConsID;
                                                                                       
                    /*if handler is current...*/
                    if (theArcCons->mHTS==theArc->mTS)
                    {
                        /*skips it...*/
                        thePrevArc=theArc;
                        theArc=theArc->mNext;
                                                        
                        /*increments countdown...*/
                        theCell->mCountdown++;
                     }
                     else
                     {
                        /*deletes list item...*/
                        if (thePrevArc!=NULL)
                        {                                                                
                            thePrevArc->mNext=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                                                                
                            theArc=thePrevArc->mNext;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                       
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }        
                        }
                        else /*it must be the first item...*/
                        {
                            theCell->mHandlerList=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                            theArc=theCell->mHandlerList;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                      
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }       
                        }
                    }         
                }
            }
        }
    }       
    //printf ("[POST-READ] Exiting...\n");
}

/*======================================
        __post_read_lcp_handler_trampoline()
=======================================*/
void __post_read_lcp_handler_trampoline (void *faulty_addr, unsigned long access_size)
{
    LCP_TCell *theCell;
    LCP_TCons *theCurrCons;
    
    if (g_mode==1)
    { 
        theCurrCons=(LCP_TCons *)g_CurrCons;
        
        theCell=rm_get_shadow_rec_fast (faulty_addr, long, LCP_TCell);
                        
        /*if old read timestamp...*/
        if (theCell->mCTSR < g_MainTS)
        {
            g_CurrCell=theCell;
            __post_read_lcp_handler_logdep (faulty_addr, access_size);
        }
    }
}

/*======================================
        __post_read_lcp_handler_logdep()
=======================================*/
void __post_read_lcp_handler_logdep (void *faulty_addr, unsigned long access_size)
{
    LCP_TCell *theNewCell, *theCell;
    LCP_TArc *theNewArc, *theArc, *thePrevArc;

    LCP_TCons *theCurrCons;

    LCP_TCons *theArcCons;
    
    theCell=g_CurrCell;
    theCurrCons=(LCP_TCons *)g_CurrCons;
    
    /*creates new arc item...*/
            pool_alloc (g_ArcPool, g_ArcPoolFreeList, theNewArc, LCP_TArc);
                        
            theNewArc->mConsID=g_CurrCons;
            theNewArc->mTS=theCurrCons->mHTS;
                                
            /*inserts item as head of handler list...*/
            theNewArc->mNext=theCell->mHandlerList;
            theCell->mHandlerList=theNewArc;
                                
            /*updates read timestamp...*/
            theCell->mCTSR=theCurrCons->mHTS;
                                
            /*increments reference count...*/
            theCurrCons->mRefCount=theCurrCons->mRefCount+1;
            //printf ("ConsID: %d \t RefCount:%u\n", g_CurrCons, theCurrCons->mRefCount);
                                
            /*decrements countdown...*/
            if (theCell->mCountdown>-1) 
                theCell->mCountdown--;
            
            /*if reached -1...*/
            if (theCell->mCountdown==-1)
            {
                /*cleans list...*/
                                                       
                thePrevArc=NULL;
                theArc=theCell->mHandlerList;

                while (theArc!=NULL)
                { 
                    theArcCons=(LCP_TCons *)theArc->mConsID;
                                                                                       
                    /*if handler is current...*/
                    if (theArcCons->mHTS==theArc->mTS)
                    {
                        /*skips it...*/
                        thePrevArc=theArc;
                        theArc=theArc->mNext;
                                                        
                        /*increments countdown...*/
                        theCell->mCountdown++;
                     }
                     else
                     {
                        /*deletes list item...*/
                        if (thePrevArc!=NULL)
                        {                                                                
                            thePrevArc->mNext=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                                                                
                            theArc=thePrevArc->mNext;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                       
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }        
                        }
                        else /*it must be the first item...*/
                        {
                            theCell->mHandlerList=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                            theArc=theCell->mHandlerList;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                      
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }       
                        }
                    }         
                }
            }
}

/*======================================
        __post_read_lcp_handler_rw_case()
=======================================*/
void __post_read_lcp_handler_rw_case (void *faulty_addr, unsigned long access_size)
{
    //unsigned long theCellIndex;

    LCP_TCell *theNewCell, *theCell;
    LCP_TArc *theNewArc, *theArc, *thePrevArc;

    LCP_TCons *theCurrCons;

    LCP_TCons *theArcCons;
        
    //printf ("[POST-READ] Entering with mode: %u\n", g_mode);
    //fflush (stdout);
            
    /*if reading from a protected cell in 'main' mode or 'atomic sequence' mode or 'final handler execution' mode...*/
    //if (g_mode==0 || g_mode==2 || g_mode==3)
    //{
        //printf ("[POST-READ HANDLER] Exiting...\n");
        /*do nothing...*/
        //return;
    //}

    #if DC_STAT == 1
    ++dc_stat.case_rw_count;
    #endif

    /*if reading from a protected cell in 'handler execution' mode...*/
    if (g_mode==1)
    /*logs dependency...*/
    { 
        /*if (g_CurrCons==NULL)
        {      
            printf ("[POST-READ] Error: g_CurrCons == NULL\n");
            exit (1);
        }*/      
            
        theCurrCons=(LCP_TCons *)g_CurrCons;
        
        //theCellIndex=faulty_addr;
        //theCell=rm_get_shadow_rec (theCellIndex);
        theCell=rm_get_shadow_rec_fast (faulty_addr, long, LCP_TCell);
                        
        /*if old read timestamp...*/
        if (theCell->mCTSR < g_MainTS)
        {
            /*if (theCell->mCTSR >= theCurrCons->mHTS)
            {
                printf ("This should not happen!\n");
                exit (1);
            }*/
            
            /*adds dependency as first item in handler list...*/
            //printf ("[POST-READ] logging dependency...\n");

            #if DC_STAT == 1
            ++dc_stat.case_rw_first_count;
            #endif

            /*creates new arc item...*/
            pool_alloc (g_ArcPool, g_ArcPoolFreeList, theNewArc, LCP_TArc);
                        
            theNewArc->mConsID=g_CurrCons;
            theNewArc->mTS=theCurrCons->mHTS;
                                
            /*inserts item as head of handler list...*/
            theNewArc->mNext=theCell->mHandlerList;
            theCell->mHandlerList=theNewArc;
                                
            /*updates read timestamp...*/
            theCell->mCTSR=theCurrCons->mHTS;
                                
            /*increments reference count...*/
            theCurrCons->mRefCount=theCurrCons->mRefCount+1;
            //printf ("ConsID: %d \t RefCount:%u\n", g_CurrCons, theCurrCons->mRefCount);
                                
            /*decrements countdown...*/
            if (theCell->mCountdown>-1) 
                theCell->mCountdown--;
            
            /*if reached -1...*/
            if (theCell->mCountdown==-1)
            {
                /*cleans list...*/
                                                       
                thePrevArc=NULL;
                theArc=theCell->mHandlerList;

                while (theArc!=NULL)
                { 
                    theArcCons=(LCP_TCons *)theArc->mConsID;
                                                                                       
                    /*if handler is current...*/
                    if (theArcCons->mHTS==theArc->mTS)
                    {
                        /*skips it...*/
                        thePrevArc=theArc;
                        theArc=theArc->mNext;
                                                        
                        /*increments countdown...*/
                        theCell->mCountdown++;
                     }
                     else
                     {
                        /*deletes list item...*/
                        if (thePrevArc!=NULL)
                        {                                                                
                            thePrevArc->mNext=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                                                                
                            theArc=thePrevArc->mNext;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                       
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }        
                        }
                        else /*it must be the first item...*/
                        {
                            theCell->mHandlerList=theArc->mNext;
                            pool_free (theArc, g_ArcPoolFreeList);
                            theArc=theCell->mHandlerList;
                                                                
                            /*decrements reference count...*/
                            theArcCons->mRefCount=theArcCons->mRefCount-1;
                            //printf ("ConsID: %d \t RefCount:%u\n", theArcConsIndex, theArcCons->mRefCount);
                                        
                            /*if reference count reaches 0 and handler deleted...*/
                            if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
                            {
                                /*deallocates slot...*/
                                pool_free (theArcCons, g_ConsPoolFreeList);                                                                      
                                //printf ("Deallocated ConsID %d\n", theArcCons);
                            }       
                        }
                    }         
                }
            }
        }
    }       
    //printf ("[POST-READ] Exiting...\n");
}

// Macro per il codice delle patch inline
#define INLINE_PATCH_CODE_W(case, reg, reg2) \
    __asm__ __volatile__ ( \
            "jmp handler_call_end_"case";" \
    ); \
    __asm__ __volatile__ ( \
        "handler_call_start_"case":" \
    /***        "call   HANDLER;" */ \
            "push   %"reg2";" \
            "push   %edx;" \
           /* "push   $0x4;"          /* parameter #2 */ \
            "push   %"reg";"        /* parameter #1 */ \
        "handler_call_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
            "addl   $0x4, %esp;"        /* remove the parameters from stack */ \
            "pop    %edx;" \
            "pop    %"reg2";" \
    /***        "call   HANDLER;" */ \
        "handler_call_end_"case":" \
    );

//macro for rw case...
#define INLINE_PATCH_CODE_RW(case, reg, reg2) \
    __asm__ __volatile__ ( \
            "jmp handler_call_end_"case";" \
    ); \
    __asm__ __volatile__ ( \
        "handler_call_start_"case":" \
        "handler_call_read_start_"case":" \
    /***        "call   HANDLER;" */ \
            "cmpl   $0x00000001, 0x08000000;"       /*address of g_mode will be inserted at runtime...*/ \
            "push   %"reg2";" \
            "push   %edx;" \
            "push   $0x4;"          /* parameter #2 */ \
            "push   %"reg";"        /* parameter #1 */ \
            "jnz     handler_call_write_start_"case";"  /*if g_mode!=1 skips calling post_read handler...*/ \
        "handler_call_read_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
          /*  "addl   $0x8, %esp;"        /* remove the parameters from stack */ \
          /*  "pop    %edx;" */ \
          /*  "pop    %"reg2";"*/ \
    /***        "call   HANDLER;" */ \
    "handler_call_write_start_"case":" \
    /***        "call   HANDLER;" */ \
         /*   "push   %"reg2";" */ \
         /*   "push   %edx;" */ \
         /*   "push   $0x4;"          /* parameter #2 */ \
         /*   "push   %"reg";"        /* parameter #1 */ \
        "handler_call_write_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
            "addl   $0x8, %esp;"        /* remove the parameters from stack */ \
            "pop    %edx;" \
            "pop    %"reg2";" \
    /***        "call   HANDLER;" */ \
            "handler_call_end_"case":" \
    );    

//alternative version of read inline patch...    
#if 0
    // Macro per il codice delle patch inline
#define INLINE_PATCH_CODE_R(case, reg, reg2) \
    __asm__ __volatile__ ( \
            "jmp handler_call_end_"case";" \
    ); \
    __asm__ __volatile__ ( \
        "handler_call_start_"case":" \
    /***        "call   HANDLER;" */ \
            "cmpl   $0x00000001, 0x08000000;"       /*address of g_mode will be inserted at runtime...*/ \
            "jnz     handler_call_end_"case";"  /*if g_mode!=1 skips calling post_read handler...*/ \
            /* begin timestamp check */ \
            "push   %edx;" \
            "mov    %"reg", %edx;" /* saves faulty addr in EDX */ \
            "handler_timestamp_k_"case":" \
            "subl    $0x08000000, %edx;" /* x - k in EBX - to be patched with k = O + B - wordsize */ \
            "handler_timestamp_factor_"case":" \
            "imull  $0x08000000, %edx;" /* (x-k)*(-sizeof(LCP_TCell)/wordsize) in EDX - to be patched */ \
            "handler_timestamp_base_"case":" \
            "addl    $0x08000000, %edx;" /* pointer to shadow_rec is now in EDX - to be patched with base */ \
            "handler_timestamp_gcellptr_"case":" \
            "movl   %edx, 0x08000000;" /* to be patched with &g_CurrCell */ \
            "mov    (%edx), %edx;" /* from shadow_rec ptr to timestamp value */ \
            "handler_timestamp_maints_"case":" \
            "cmpl    0x08000000, %edx;" /* comparison with main_ts - to be patched with &g_mainTS */ \
            "jae    handler_call_pre_end_"case";" /* skips post-read handler if current timestamp */ \
            /* end timestamp check */ \
            "push   %"reg2";" \
            "push   %"reg";"        /* parameter #1 */ \
            "handler_call_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
            "addl   $0x4, %esp;"        /* remove the parameters from stack */ \
            "pop    %"reg2";" \
            "handler_call_pre_end_"case":" \
            "pop    %edx;" \
    /***        "call   HANDLER;" */ \
        "handler_call_end_"case":" \
    );
#endif    
    
#if 0
// Macro per il codice delle patch inline
#define INLINE_PATCH_CODE_R(case, reg, reg2) \
    __asm__ __volatile__ ( \
            "jmp handler_call_end_"case";" \
    ); \
    __asm__ __volatile__ ( \
        "handler_call_start_"case":" \
    /***        "call   HANDLER;" */ \
            "cmpl   $0x00000001, 0x08000000;"       /*address of g_mode will be inserted at runtime...*/ \
            "jnz     handler_call_end_"case";"  /*if g_mode!=1 skips calling post_read handler...*/ \
            /* begin timestamp check */ \
            "push   %edx;" \
            "push   %ebx;" \
            "mov    %"reg", %edx;" /* saves faulty addr in EDX */ \
            "leal   (%edx, %edx, 4), %ebx;" \
            "mov    $0x3fffffec, %edx;" \
            "sub    %ebx, %edx;" \
            "pop    %ebx;" \
            "handler_timestamp_gcellptr_"case":" \
            "movl   %edx, 0x08000000;" /* to be patched with &g_CurrCell */ \
            "mov    (%edx), %edx;" /* from shadow_rec ptr to timestamp value */ \
            "handler_timestamp_maints_"case":" \
            "cmpl    0x08000000, %edx;" /* comparison with main_ts - to be patched with &g_mainTS */ \
            "jae    handler_call_pre_end_"case";" /* skips post-read handler if current timestamp */ \
            /* end timestamp check */ \
            "push   %"reg2";" \
            "push   %"reg";"        /* parameter #1 */ \
            "handler_call_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
            "addl   $0x4, %esp;"        /* remove the parameters from stack */ \
            "pop    %"reg2";" \
            "handler_call_pre_end_"case":" \
            "pop    %edx;" \
    /***        "call   HANDLER;" */ \
        "handler_call_end_"case":" \
    );
#endif

// Macro per il codice delle patch inline
#define INLINE_PATCH_CODE_R(case, reg, reg2) \
    __asm__ __volatile__ ( \
            "jmp handler_call_end_"case";" \
    ); \
    __asm__ __volatile__ ( \
        "handler_call_start_"case":" \
    /***        "call   HANDLER;" */ \
            "cmpl   $0x00000001, 0x08000000;"       /*address of g_mode will be inserted at runtime...*/ \
            "jnz     handler_call_end_"case";"  /*if g_mode!=1 skips calling post_read handler...*/ \
            /* begin timestamp check */ \
            "push   %edx;" \
            "mov    %"reg", %edx;" /* saves faulty addr in EDX */ \
            "leal   (%edx, %edx, 4), %edx;" \
            "neg    %edx;" \
            "add    $0x3fffffec, %edx;" \
            "handler_timestamp_gcellptr_"case":" \
            "movl   %edx, 0x08000000;" /* to be patched with &g_CurrCell */ \
            "mov    (%edx), %edx;" /* from shadow_rec ptr to timestamp value */ \
            "handler_timestamp_maints_"case":" \
            "cmpl    0x08000000, %edx;" /* comparison with main_ts - to be patched with &g_mainTS */ \
            "jae    handler_call_pre_end_"case";" /* skips post-read handler if current timestamp */ \
            /* end timestamp check */ \
            "push   %"reg2";" \
            "push   %"reg";"        /* parameter #1 */ \
            "handler_call_"case":" \
            "movl   $0x08000000, %"reg";"   /* l'indirizzo di memoria sarà modificato */ \
            "call   *%"reg";" \
            "addl   $0x4, %esp;"        /* remove the parameters from stack */ \
            "pop    %"reg2";" \
            "handler_call_pre_end_"case":" \
            "pop    %edx;" \
    /***        "call   HANDLER;" */ \
        "handler_call_end_"case":" \
    );

// Funzione per l'inserimento degli indirizzi degli handler nel codice delle patch inline
// N.B. questa funzione contiene anche il codice delle patch inline (non viene mai eseguito, serve solo per essere copiato)
void inline_patch() {
    // Sprotegge pagina del codice delle patch inline
    unsigned int page_start = ((unsigned int)&handler_call_start_RA & ~(getpagesize()-1));
    unsigned int page_end = ((unsigned int)(&handler_call_end_RWC-1) & ~(getpagesize()-1)) + getpagesize();
    if(-1 == mprotect((void*)page_start, page_end-page_start, PROT_READ|PROT_WRITE|PROT_EXEC)) {
        printf("[inline_patch]: mprotect() failed!\n");
        exit(1);
    }
    
    // Patch del codice delle patch inline (inserimento degli indirizzi degli handler)
    unsigned read_handler_addr = (unsigned)__post_read_lcp_handler;
    unsigned write_handler_addr = (unsigned)__post_write_lcp_handler;
    *(unsigned*)(&handler_call_RA + 1) = read_handler_addr;
    *(unsigned*)(&handler_call_RC + 1) = read_handler_addr;
    *(unsigned*)(&handler_call_WA + 1) = write_handler_addr;
    *(unsigned*)(&handler_call_WC + 1) = write_handler_addr;
    
    //multiple access case...
    *(unsigned*)(&handler_call_read_RWA + 1) = __post_read_lcp_handler_rw_case;
    *(unsigned*)(&handler_call_read_RWC + 1) = __post_read_lcp_handler_rw_case;
    *(unsigned*)(&handler_call_write_RWA + 1) = write_handler_addr;
    *(unsigned*)(&handler_call_write_RWC + 1) = write_handler_addr;
    
    /*printf ("func 1: %p\n", &handler_call_start_RA);
    printf ("func 2: %p\n", &handler_call_start_RC);
    printf ("func 3: %p\n", &handler_call_start_WA);
    printf ("func 4: %p\n", &handler_call_start_WC);*/
    
    //copies address of g_mode...
    *(unsigned*)(&handler_call_start_RA + 2) = &g_mode;
    *(unsigned*)(&handler_call_start_RC + 2) = &g_mode;
    
    //multiple access case...
    *(unsigned*)(&handler_call_start_RWA + 2) = &g_mode;
    *(unsigned*)(&handler_call_start_RWC + 2) = &g_mode;
    
    //timestamp check...
    // k = O + B - word_size
    //*(unsigned*)(&handler_timestamp_k_RA + 2) = rm_OFFSET+rm_heap_START_BRK-4;
    //*(unsigned*)(&handler_timestamp_k_RC + 2) = rm_OFFSET+rm_heap_START_BRK-4;
    // factor = - shadow_rec_size / word_size
    //*(unsigned*)(&handler_timestamp_factor_RA + 2) = -1*(long)sizeof (LCP_TCell)/4;
    //*(unsigned*)(&handler_timestamp_factor_RC + 2) = -1*(long)sizeof (LCP_TCell)/4;
    // base = B
    //*(unsigned*)(&handler_timestamp_base_RA + 2) = rm_heap_START_BRK;
    //*(unsigned*)(&handler_timestamp_base_RC + 2) = rm_heap_START_BRK;
    // &g_CurrCell
    *(unsigned*)(&handler_timestamp_gcellptr_RA + 2) = &g_CurrCell;
    *(unsigned*)(&handler_timestamp_gcellptr_RC + 2) = &g_CurrCell;
    //g_MainTS
    *(unsigned*)(&handler_timestamp_maints_RA + 2) = &g_MainTS;
    *(unsigned*)(&handler_timestamp_maints_RC + 2) = &g_MainTS;
    
    // Reimposta protezione pagina del codice delle patch inline
    if(-1 == mprotect((void*)page_start, page_end-page_start, PROT_READ|PROT_EXEC)) {
        printf("[inline_patch]: mprotect() failed!\n");
        exit(1);
    }
    
    // Codice delle patch inline (non viene mai eseguito, serve solo per essere copiato)
    INLINE_PATCH_CODE_R("RA", "eax", "ecx")
    INLINE_PATCH_CODE_R("RC", "ecx", "eax")
    INLINE_PATCH_CODE_W("WA", "eax", "ecx")
    INLINE_PATCH_CODE_W("WC", "ecx", "eax")
    INLINE_PATCH_CODE_RW("RWA", "eax", "ecx")
    INLINE_PATCH_CODE_RW("RWC", "ecx", "eax")
    
    return;
}

// ---------------------------------------------------------------------
//  _dc_init
// ---------------------------------------------------------------------
void _dc_init (comparator_t inComparator)
{
    //initializes memory pools...
    g_ArcPool=pool_init (MEM_POOL_PAGE_SIZE, sizeof (LCP_TArc), &g_ArcPoolFreeList);
    g_WrCellsPool=pool_init (MEM_POOL_PAGE_SIZE, sizeof (LCP_TWrCells), &g_WrCellsPoolFreeList);
    g_ConsPool=pool_init (MEM_POOL_PAGE_SIZE, sizeof (LCP_TCons), &g_ConsPoolFreeList);
    g_ExConsPool=pool_init (MEM_POOL_PAGE_SIZE, sizeof (LCP_TExecutedCons), &g_ExConsPoolFreeList);
    
    //patch & size tables...
    void* patch_table[rm_patch_num][rm_size_num];
    size_t size_table[rm_patch_num][rm_size_num];

    // Inserimento degli indirizzi degli handler nel codice delle patch inline
    inline_patch();
    
    // Creazione tabelle per patch inline
    patch_table[rm_read_eax_patch][rm_size_4] = &handler_call_start_RA;
    size_table[rm_read_eax_patch][rm_size_4] = &handler_call_end_RA - &handler_call_start_RA;
    patch_table[rm_read_ecx_patch][rm_size_4] = &handler_call_start_RC;
    size_table[rm_read_ecx_patch][rm_size_4] = &handler_call_end_RC - &handler_call_start_RC;
    
    patch_table[rm_write_eax_patch][rm_size_4] = &handler_call_start_WA;
    size_table[rm_write_eax_patch][rm_size_4] = &handler_call_end_WA - &handler_call_start_WA;
    patch_table[rm_write_ecx_patch][rm_size_4] = &handler_call_start_WC;
    size_table[rm_write_ecx_patch][rm_size_4] = &handler_call_end_WC - &handler_call_start_WC;
    
    patch_table[rm_rd_wr_eax_patch][rm_size_4] = &handler_call_start_RWA;
    size_table[rm_rd_wr_eax_patch][rm_size_4] = &handler_call_end_RWA - &handler_call_start_WA;
    patch_table[rm_rd_wr_ecx_patch][rm_size_4] = &handler_call_start_RWC;
    size_table[rm_rd_wr_ecx_patch][rm_size_4] = &handler_call_end_RWC - &handler_call_start_RWC;
    
    //initializes rm...
    #if DC_INLINE_PATCH == 1
    rm_init (NULL, NULL, patch_table, size_table, sizeof (LCP_TCell), 4);
    #endif
    
    #if DC_INLINE_PATCH == 0
    rm_init (__post_read_lcp_handler, __post_write_lcp_handler, NULL, NULL, sizeof (LCP_TCell), 4);
    #endif
    
    #if DC_INLINE_PATCH == 2
    rm_init (__post_read_lcp_handler_trampoline, __post_write_lcp_handler, NULL, NULL, sizeof (LCP_TCell), 4);
    #endif
        
    /*initializes LScheduler...*/
    LScheduler_Init (inComparator);
        
    //allocates dummy cell...
    g_DummyPtr=rm_malloc (4);
    g_DummyCellIndex=g_DummyPtr;
    
    // flag dc was initialized
    g_initialized = 1;

    #if DC_STAT == 1
    printf("[dc] ### warning: dc compiled with DC_STAT == 1\n");
    #endif
}


// ---------------------------------------------------------------------
//  newcons
// ---------------------------------------------------------------------
int newcons (LCP_THandler inHandler, void *inParam)
{ 
    LCP_TCons *theNewCons;        
    LCP_TCell *theDummyCell;
    LCP_TArc *theDummyArc;
        
    unsigned long theAux;
        
    if (!g_initialized) _dc_init(NULL);

    g_ConsCount++;
    pool_alloc (g_ConsPool, g_ConsPoolFreeList, theNewCons, LCP_TCons);
             
    theNewCons->mHandler=inHandler;
    theNewCons->mParam=inParam;
        
    theNewCons->mFinalHandler=NULL;
    theNewCons->mFinalScheduled=0;
        
    theNewCons->mHTS=g_MainTS;
    theNewCons->mRefCount=1; /*referenced from dummy arc...*/
    theNewCons->mConsInfo=NULL;
        
    theDummyCell=rm_get_shadow_rec_fast (g_DummyCellIndex, long, LCP_TCell);
               
    pool_alloc (g_ArcPool, g_ArcPoolFreeList, theDummyArc, LCP_TArc);
                        
    theDummyArc->mConsID=(int)theNewCons;
    theDummyArc->mTS=theNewCons->mHTS;
    theDummyArc->mNext=theDummyCell->mHandlerList;
                               
    theDummyCell->mHandlerList=theDummyArc;
                        
    /*executes callback handler...*/
    if (g_NewConsCallback!=NULL)
        g_NewConsCallback (theNewCons);
        
    /*triggers first constraint execution by changing value of dummy cell...*/
    /*cannot install 2^32 constraints from within a single constraint!!!*/
    theAux=theDummyCell->mData;
    theAux++;
    *g_DummyPtr=theAux;
        
    return (int)theNewCons;
}

// ---------------------------------------------------------------------
//  schedule_final
// ---------------------------------------------------------------------
int schedule_final (int cons_id, LCP_THandler inFinalHandler)
{   
    //if in handler execution mode...
    if (g_mode==1)
    {
        LCP_TCons *cons_ptr=(LCP_TCons *)cons_id;
        
        cons_ptr->mFinalHandler=inFinalHandler;
        
        if (inFinalHandler!=NULL && cons_ptr->mFinalScheduled==0)
        {
            LCP_TExecutedCons *theNewEntry;
            cons_ptr->mFinalScheduled=1;
            pool_alloc (g_ExConsPool, g_ExConsPoolFreeList, theNewEntry, LCP_TExecutedCons);
            theNewEntry->mConsIndex=cons_ptr;
            theNewEntry->mNext=g_ExecutedConsList;
            g_ExecutedConsList=theNewEntry;
        }
        
        return 0;
    }
    else
    {
        return -1;
    }
}

// ---------------------------------------------------------------------
//  get_cons_id
// ---------------------------------------------------------------------
int get_cons_id()
{
    if (g_mode==1 && g_CurrCons!=NULL)
        return g_CurrCons;
    else
        return -1;
}

// ---------------------------------------------------------------------
//  delcons
// ---------------------------------------------------------------------
void delcons (int inConsID)
{
    /*Warning: a constraint should not delete itself!!!*/
        
    LCP_TCons *theCurrCons;
        
    theCurrCons=(LCP_TCons *)inConsID;
        
    if (!g_initialized) _dc_init(NULL);

    #if 0
    if (theCurrCons->mFinalScheduled==1)
    {
        printf ("[DEBUG] delcons() ignored (reason: handler scheduled for final execution)\n");
        return;
    }
    #endif
        
    /*so that deletecons callback will NOT delete constraint...*/
    theCurrCons->mRefCount++;
        
    /*executes callback handler...*/
    if (g_DeleteConsCallback!=NULL)
        g_DeleteConsCallback (inConsID);
                
    theCurrCons->mRefCount--;
        
    /* if no references and not scheduled for final execution.... */
    if (theCurrCons->mRefCount==0 && theCurrCons->mFinalScheduled != 1)
    {
        /*deallocates constraint...*/
        pool_free (theCurrCons, g_ConsPoolFreeList);
        //printf ("Deallocated ConsID %d\n", inConsID);
    }
    else
        /*sets timestamp to conventional value -2...*/
        theCurrCons->mHTS=-2;

    // decrement constraint counter
    g_ConsCount--;
}


// ---------------------------------------------------------------------
//  _dc_cleanup
// ---------------------------------------------------------------------
void _dc_cleanup ()
{
    //deletes memory pools...
    pool_cleanup (g_ArcPool);
    pool_cleanup (g_WrCellsPool);
    pool_cleanup (g_ConsPool);
    pool_cleanup (g_ExConsPool);
	
    //deallocates dummy cell...
    rm_free (g_DummyPtr);
        
    /*cleans up LScheduler...*/
    LScheduler_CleanUp ();
}


// ---------------------------------------------------------------------
//  LCP_GetFirstArc
// ---------------------------------------------------------------------
void* LCP_GetFirstArc (unsigned long inCellID, int *outConsID)
{
    LCP_TCell *theCell;
    LCP_TArc *theArc;
    LCP_TCons *theArcCons;        
        
    //printf ("Entering GetFirstArc()...\n");
        
    theCell=rm_get_shadow_rec_fast (inCellID, long, LCP_TCell);
        
    /*resets countdown...*/
    /*Warning: it is assumed that the scheduler will scan the entire list...*/
    theCell->mCountdown=0;
        
    /*if empty list...*/
    if (theCell->mHandlerList==NULL)
    {
        //printf ("Exiting GetFirstArc()...\n");
        return NULL; /*returns null iterator...*/
    }
                
    theArc=theCell->mHandlerList;
                                        
    theArcCons=(LCP_TCons *)theArc->mConsID;
        
    //if cons is not current (no logical arc exists) drop arc and iterate...
    //if cons already scheduled...action is left to the scheduler...
    //else return it
        
    while (theArcCons->mHTS!=theArc->mTS)
    {
        theCell->mHandlerList=theArc->mNext;
        pool_free (theArc, g_ArcPoolFreeList);
        theArcCons->mRefCount--; 
        //printf ("ConsID: %d \t RefCount:%d\n", theArcConsIndex, theArcCons->mRefCount);
                                        
        /*if reference count reaches 0 and handler deleted...*/
        if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
        {
            /*deallocates slot...*/
            pool_free (theArcCons, g_ConsPoolFreeList);
            //printf ("Deallocated ConsID %d\n", theArcConsIndex);
        }
                
        if (theCell->mHandlerList==NULL)
        {
            //printf ("Exiting GetFirstArc()...\n");
            return NULL; /*returns null iterator...*/
        }        
                
        theArc=theCell->mHandlerList;
                                        
        theArcCons=(LCP_TCons *)theArc->mConsID;
    }
        
    /*increments countdown...*/
    theCell->mCountdown++;
        
    *outConsID=theArcCons;
        
    theCell->mHandlerList=theArc->mNext;
    pool_free (theArc, g_ArcPoolFreeList);
    theArcCons->mRefCount--; 
    //printf ("ConsID: %d \t RefCount:%d\n", theArcConsIndex, theArcCons->mRefCount);
                                        
    /*if reference count reaches 0 and handler deleted...*/
    if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
    {
        /*deallocates slot...*/
        pool_free (theArcCons, g_ConsPoolFreeList);
        //printf ("Deallocated ConsID %d\n", theArcConsIndex);
    }
        
    //printf ("Exiting GetFirstArc()...\n");
        
    /*returns cellID as iterator...*/
    return (void *)inCellID;
}


// ---------------------------------------------------------------------
//  LCP_GetNextArc
// ---------------------------------------------------------------------
void* LCP_GetNextArc  (void *inIterator, int *outConsID)
{
    LCP_TCell *theCell;
    LCP_TArc *theArc;
    LCP_TCons *theArcCons;        
    unsigned long theArcIndex, theArcConsIndex;
        
    //printf ("Entering GetNextArc()...\n");
    
    theCell=rm_get_shadow_rec_fast (inIterator, long, LCP_TCell);

    /*if empty list...*/
    if (theCell->mHandlerList==NULL)
    {
        //printf ("Exiting GetNextArc()...\n");
        return NULL; /*returns null iterator...*/
    }
                
    theArc=theCell->mHandlerList;
                                        
    theArcCons=(LCP_TCons *)theArc->mConsID;
        
    //if cons is not current (no logical arc exists) drop arc and iterate
    //if cons already scheduled...action is left to the scheduler...
    //else return it
        
    while (theArcCons->mHTS!=theArc->mTS)
    {
        theCell->mHandlerList=theArc->mNext;
        pool_free (theArc, g_ArcPoolFreeList);
        theArcCons->mRefCount--; 
        //printf ("ConsID: %d \t RefCount:%d\n", theArcConsIndex, theArcCons->mRefCount);
                                        
        /*if reference count reaches 0 and handler deleted...*/
        if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
        {
            /*deallocates slot...*/
            pool_free (theArcCons, g_ConsPoolFreeList);
            //printf ("Deallocated ConsID %d\n", theArcConsIndex);
        }
                
        if (theCell->mHandlerList==NULL)
        {
            //printf ("Exiting GetNextArc()...\n");
            return NULL; /*returns null iterator...*/
        }
                        
        theArc=theCell->mHandlerList;
                                        
        theArcCons=(LCP_TCons *)theArc->mConsID;
    }
        
    /*increments countdown...*/
    theCell->mCountdown++;
        
    *outConsID=theArcCons;
        
    theCell->mHandlerList=theArc->mNext;
    pool_free (theArc, g_ArcPoolFreeList);
    theArcCons->mRefCount--; 
    //printf ("ConsID: %d \t RefCount:%d\n", theArcConsIndex, theArcCons->mRefCount);
                                        
    /*if reference count reaches 0 and handler deleted...*/
    if (theArcCons->mRefCount==0 && theArcCons->mHTS==-2)
    {
        /*deallocates slot...*/
        pool_free (theArcCons, g_ConsPoolFreeList);
        //printf ("Deallocated ConsID %d\n", theArcConsIndex);
    }
        
    //printf ("Exiting GetNextArc()...\n");
        
    /*returns cellID as iterator...*/
    return (void *)inIterator;
}


// ---------------------------------------------------------------------
//  LCP_InstallNewConsCallback
// ---------------------------------------------------------------------
void LCP_InstallNewConsCallback (LCP_TCallback inCallbackFunc)
{
    g_NewConsCallback=inCallbackFunc;
}


// ---------------------------------------------------------------------
//  LCP_InstallDeleteConsCallback
// ---------------------------------------------------------------------
void LCP_InstallDeleteConsCallback (LCP_TCallback inCallbackFunc)
{
    g_DeleteConsCallback=inCallbackFunc;
}


// ---------------------------------------------------------------------
//  LCP_GetConsInfo
// ---------------------------------------------------------------------
void* LCP_GetConsInfo (int inConsID)
{
    LCP_TCons *theCons;
                
    theCons=(LCP_TCons *)inConsID;
                
    return theCons->mConsInfo;
}


// ---------------------------------------------------------------------
//  SetConsInfo
// ---------------------------------------------------------------------
void LCP_SetConsInfo (int inConsID, void *inInfo)
{
    LCP_TCons *theCons;
                
    theCons=(LCP_TCons *)inConsID;
        
    /*if adding info...*/
    if (theCons->mConsInfo==NULL && inInfo!=NULL)
    {
        theCons->mConsInfo=inInfo;
        theCons->mRefCount++;
                                
        return;
    }
        
    /*if removing info...*/
    if (theCons->mConsInfo!=NULL && inInfo==NULL)
    {
        theCons->mConsInfo=inInfo;
        theCons->mRefCount--;
                
        /*if last reference removed and handler scheduled for deletion...*/
        if (theCons->mRefCount==0 && theCons->mHTS==-2)
        {
            /*deallocates constraint...*/
            pool_free (theCons, g_ConsPoolFreeList);
            //printf ("Deallocated ConsID %d\n", inConsID);
        }
                                
        return;
    }
        
    /*if modifying info...*/
    if (theCons->mConsInfo!=NULL && inInfo!=NULL)
    {
        theCons->mConsInfo=inInfo; 
                
        return;
    }
}


// ---------------------------------------------------------------------
//  begin_at
// ---------------------------------------------------------------------
int begin_at () {

    /*if in 'main' mode...*/
    if (g_mode==0)
    {
        /*switches to 'atomic sequence' mode...*/
        g_mode=2;
        g_AtomicCount++;
        return 0;
    }
	
    /*if already in 'atomic execution' mode...*/
    if (g_mode==2)
    {
        if (g_AtomicCount<0xffffffff)
            g_AtomicCount++;
        else
        {
            printf ("[DEBUG] Error: more than 2^32 nested atomic sequences!\n");
            return -1;
        }
		
        return 0;
    }
	
    if (g_mode==1)
    {
        //printf ("[DEBUG] Error: begin_at() should not be called from 'handler execution' mode!\n");
        return -1;
    }
}


// ---------------------------------------------------------------------
//  end_at
// ---------------------------------------------------------------------
int end_at() {
    unsigned long theAux;
	
    if (g_AtomicCount==0) {
        //printf ("[DEBUG] Error: no matching end_at() for LCP_EndAtomicSeq() call!\n");
        return -1;
    }
	
    g_AtomicCount--;
	
    if (g_AtomicCount==0) {
        LCP_TCell *theDummyCell=rm_get_shadow_rec_fast (g_DummyCellIndex, long, LCP_TCell);
		
        /*switches back to main mode...*/
        g_mode=0;
        /*touches dummy cell so that written cells list can be processed...*/
        theAux=theDummyCell->mData;
        theAux++;
        *g_DummyPtr=theAux;
        return 0;
    }	
}


// ---------------------------------------------------------------------
//  LCP_GetConsTimeStamp
// ---------------------------------------------------------------------
unsigned long LCP_GetConsTimeStamp (int inConsID) {
    LCP_TCons *theCons;
    theCons=(LCP_TCons *)inConsID;
    return theCons->mHTS;
}


// ---------------------------------------------------------------------
//  LCP_GetConsParam
// ---------------------------------------------------------------------
void* LCP_GetConsParam (int inConsID) {
    LCP_TCons *theCons;
    theCons=(LCP_TCons *)inConsID;
    return theCons->mParam;
}


// ---------------------------------------------------------------------
//  dc_dump_arcs
// ---------------------------------------------------------------------
void dc_dump_arcs(void* addr) {
    LCP_TCell* cell;
    LCP_TArc * arc;
    printf("[dc] summary for address %p\n", addr);
    if (!rm_is_reactive(addr)) {
        printf("[dc]     - none (non-reactive address)\n");
        return;
    }
    cell = rm_get_shadow_rec_fast(addr, long, LCP_TCell);
    printf("[dc]     - write timestamp %lu\n", cell->mCTSW);
    printf("[dc]     - read timestamp %lu\n", cell->mCTSR);
    printf("[dc]     - countdown %ld\n", cell->mCountdown);
    printf("[dc]     - data %lu (%p)\n", cell->mData, (void*)cell->mData);
    for (arc = cell->mHandlerList; arc != NULL; arc = arc->mNext)
        printf("[dc]     - arc %p [cons=%p, handler=%p, param=%p, refcount=%lu, arc timestamp=%lu, %s] \n", 
            arc,
            (void*)arc->mConsID, 
            ((LCP_TCons*)arc->mConsID)->mHandler,
            ((LCP_TCons*)arc->mConsID)->mParam,
            ((LCP_TCons*)arc->mConsID)->mRefCount,
            arc->mTS, 
            ((LCP_TCons*)arc->mConsID)->mHTS > arc->mTS ? "stale" : "up to date");
}

	
// ---------------------------------------------------------------------
//  dc_num_cons
// ---------------------------------------------------------------------
unsigned long dc_num_cons() {
    return g_ConsCount;
}


// ---------------------------------------------------------------------
//  dc_num_exec_cons
// ---------------------------------------------------------------------
unsigned long dc_num_exec_cons() {
    return g_num_exec_cons;
}


// ---------------------------------------------------------------------
//  rmalloc
// ---------------------------------------------------------------------
void* rmalloc(size_t size) {
    if (!g_initialized) _dc_init(NULL);
    return rm_malloc(size);
}


// ---------------------------------------------------------------------
//  rcalloc
// ---------------------------------------------------------------------
void* rcalloc(size_t num, size_t size) {
    if (!g_initialized) _dc_init(NULL);
    return rm_calloc(num, size);
}


// ---------------------------------------------------------------------
//  rrealloc
// ---------------------------------------------------------------------
void* rrealloc(void* ptr, size_t size) {
    if (!g_initialized) _dc_init(NULL);
    return rm_realloc(ptr, size);
}


// ---------------------------------------------------------------------
//  rfree
// ---------------------------------------------------------------------
void rfree(void* ptr) {
    if (!g_initialized) _dc_init(NULL);
    rm_free(ptr);
}


/* Copyright (C) 2008-2010 Camil Demetrescu, Andrea Ribichini

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
