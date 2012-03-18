/* =====================================================================
 *  burst.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 9, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/


#ifndef __BURST__
#define __BURST__

#include "driver.h"

// constants
#define NULL_FRAME ((void*)0xFFFFFFFF)


// typedefs
typedef struct {
    UINT32 routine_id;
    UINT16 call_site;
} stack_frame_t;


// external globals
extern stack_frame_t shadow_stack[STACK_MAX_DEPTH];
extern UINT32        curr_frame_idx;


// ---------------------------------------------------------------------
//  get_first_frame
// ---------------------------------------------------------------------
// get pointer to first frame in call chain
// prototype (defined here as macro): 
//     void* get_first_frame();
#define get_first_frame() ((void*)(curr_frame_idx-1))


// ---------------------------------------------------------------------
//  get_next_frame
// ---------------------------------------------------------------------
// get reference to next frame in call chain
// returns pointer to next frame in call chain, or NULL_FRAME if
// no more frame remain
// prototype (defined here as macro): 
//    void* get_next_frame(void* frame);
#define get_next_frame(f) \
    ((UINT32)(f) == 0 ? NULL_FRAME : (void*)((UINT32)(f)-1))


// ---------------------------------------------------------------------
//  get_frame_info
// ---------------------------------------------------------------------
// get info stored in call frame
// prototype (defined here as macro): 
//     void get_call_frame(
//         void* frame, UINT32 *routine_id, UINT16 *call_site);
#define get_call_frame(f, rtn, cs) {             \
    *(rtn) = shadow_stack[(UINT32)(f)].routine_id; \
    *(cs)  = shadow_stack[(UINT32)(f)].call_site;  \
}


// ---------------------------------------------------------------------
//  do_stack_walk
// ---------------------------------------------------------------------
// climbs the shadow call stack and calls f for each stack frame
// in the right order (from stack base to stack top)
#if ADAPTIVE==1

#if WEIGHT_COMPENSATION==1
void do_stack_walk(void (*f)(UINT32 routine_id,
                            UINT16 call_site, UINT16 increment), 
                    int (*ctxt)(UINT32 routine_id, UINT16 call_site));
#else
void do_stack_walk(void (*f)(UINT32 routine_id, UINT16 call_site),
                    int (*ctxt)(UINT32 routine_id, UINT16 call_site));
#endif
    
#else
void do_stack_walk(void (*f)(UINT32 routine_id, UINT16 call_site));
#endif


// ---------------------------------------------------------------------
//  rtn_enter_burst
// ---------------------------------------------------------------------
void rtn_enter_burst(UINT32 routine_id, UINT16 call_site);


// ---------------------------------------------------------------------
//  rtn_exit_burst
// ---------------------------------------------------------------------
void rtn_exit_burst();


// ---------------------------------------------------------------------
//  tick_burst
// ---------------------------------------------------------------------
void tick_burst();


// ---------------------------------------------------------------------
//  init_burst
// ---------------------------------------------------------------------
int init_burst(int argc, char** argv);


// ---------------------------------------------------------------------
//  cleanup_burst
// ---------------------------------------------------------------------
void cleanup_burst();


// ---------------------------------------------------------------------
//  write_output_burst
// ---------------------------------------------------------------------
void write_output_burst(logfile_t* f, driver_t* d);

#endif


/* Copyright (C) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
*/
