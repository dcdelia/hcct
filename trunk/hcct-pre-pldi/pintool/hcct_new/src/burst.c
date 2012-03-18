/* =====================================================================
 *  burst.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 9, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/20 07:22:13 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.12 $
*/


#include "cct.h"
#include "analysis.h"
#include "burst.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if ADAPTIVE==1
#include <time.h>
#endif


// constants
enum { NO, MAYBE, YES };

// external globals
stack_frame_t shadow_stack[STACK_MAX_DEPTH];    // shadow stack
UINT32        curr_frame_idx;                   // shadow stack pointer


// local globals
static UINT32 sampling_rate;  // number of ticks between sampling points
static UINT32 burst_duration; // duration of a burst in number of ticks
                              // where burst_duration << sampling_rate
                              // and sampling_rate is a multiple of
                              // burst duration
static UINT64 tick_counter;   // master tick counter
static UINT64 num_bursts;     // number of performed bursts
static UINT64 num_rtn_enter;  // actual number of performed rtn_enter
static UINT64 num_rtn_exit;   // actual number of performed rtn_exit
static int    bursting_on;    // 1 if a burst is currently active

#if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
static int    weight_comp_on; // 1 if weight compensation is needed
#endif


// ---------------------------------------------------------------------
//  do_stack_walk
// ---------------------------------------------------------------------
// climbs the shadow call stack and calls f for each stack frame
// in the right order (from stack base to stack top)
#if ADAPTIVE==1

#if WEIGHT_COMPENSATION==1
void do_stack_walk(void (*f)(UINT32 routine_id,
                            UINT16 call_site, UINT16 increment), 
                    int (*ctxt)(UINT32 routine_id, UINT16 call_site)) {
#else
void do_stack_walk(void (*f)(UINT32 routine_id, UINT16 call_site),
                    int (*ctxt)(UINT32 routine_id, UINT16 call_site)) {
#endif
    
#else
void do_stack_walk(void (*f)(UINT32 routine_id, UINT16 call_site)) {
#endif

    #if ADAPTIVE==0
    UINT32 num_frames, i;
    stack_frame_t buf[STACK_MAX_DEPTH];
    void* frame;

    // walk the simulated stack -- use get_first_frame, get_next_frame
    // and get_call_frame as this is what a real implementation would do
    num_frames = 0;
    frame = get_first_frame();    
    while (frame != NULL_FRAME) {

        // get info about current frame
        get_call_frame(frame, &buf[num_frames].routine_id, 
                              &buf[num_frames].call_site);

        // get up to parent frame in call stack
        frame = get_next_frame(frame);

        // increment frames count
        num_frames++;
    }

    // issue rtn_enter events (scan buf[] in reverse order)
    for (i = num_frames; i > 0; ) {
        i--;
        //~ #if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
        //~ if (weight_comp_on==YES) {
            //~ // increment by RE_ENABLING (1/RR)
            //~ f(buf[i].routine_id, buf[i].call_site, RE_ENABLING);
        //~ } else {
            //~ // increment by 1
            //~ f(buf[i].routine_id, buf[i].call_site, 1);
        //~ }
        //~ #else
        f(buf[i].routine_id, buf[i].call_site);
        //~ #endif
    }
    #else
    // ACCT - fast stack scan with embedded context lookup
    int i;
    
    // finding "parent" context
    for (i=0; i<curr_frame_idx-1; ++i) {
        #if WEIGHT_COMPENSATION==1
        if (weight_comp_on==YES) {
            // increment by RE_ENABLING (1/RR)
            f(shadow_stack[i].routine_id,
                shadow_stack[i].call_site, RE_ENABLING);
        } else {
            // increment by 1
            f(shadow_stack[i].routine_id,
                shadow_stack[i].call_site, 1);
        }
        #else
        // increment by 1
        f(shadow_stack[i].routine_id, shadow_stack[i].call_site);
        #endif
    }
    
    if (ctxt(shadow_stack[curr_frame_idx-1].routine_id,
                shadow_stack[curr_frame_idx-1].call_site)==1) {
        //~ // context already in the tree
        //~ #if RE_ENABLING==RAND_MAX
        //~ //printf("DISCARD\n");   
        //~ bursting_on=NO; 
        //~ #else
        if ((rand()%RE_ENABLING)==0) {
                //printf("RE-ENABLING\n");
                bursting_on=YES;
                num_bursts++;
                #if WEIGHT_COMPENSATION==1
                weight_comp_on = YES; // weight compensation enabled
                f(shadow_stack[i].routine_id,
                    shadow_stack[i].call_site, RE_ENABLING);
                #else
                f(shadow_stack[i].routine_id,
                    shadow_stack[i].call_site);
                #endif
                num_rtn_enter++;
            } else {
                //printf("DISCARD\n");   
                bursting_on=NO;
            }
        //~ #endif
    } else {
                // new context found
                //printf("NEW\n");
                bursting_on=YES;
                num_bursts++;
                #if WEIGHT_COMPENSATION==1
                weight_comp_on = NO;
                f(shadow_stack[i].routine_id,
                    shadow_stack[i].call_site, 1);
                #else
                f(shadow_stack[i].routine_id,
                    shadow_stack[i].call_site);
                #endif
                num_rtn_enter++;
    }
    #endif
}

//~ #if ADAPTIVE==1
//~ // ---------------------------------------------------------------------
//~ //  context_lookup
//~ // ---------------------------------------------------------------------
//~ int context_lookup(UINT32 routine_id, UINT16 call_site) {
    //~ #if EMPTY==1
    //~ return 0;
    //~ #else
    //~ int i, depth=0;
    //~ #if COMPARE_TO_CCT==0
    //~ cct_node_t *parent=cct_get_root(), *node;
    //~ #else
    //~ cct_node_t *parent=hcct_get_root(), *node;
    //~ #endif
    //~ for (i=0; i==depth && i<curr_frame_idx; ++i) {
        //~ for (node=parent->first_child; node!=NULL; node=node->next_sibling) {
            //~ if (shadow_stack[i].routine_id == node->routine_id &&
                //~ shadow_stack[i].call_site == node->call_site) {
                    //~ parent=node;
                    //~ ++depth;
                    //~ break;
            //~ }
        //~ }            
    //~ }
    //~ return (depth==curr_frame_idx);
    //~ #endif
//~ }
//~ #endif


// ---------------------------------------------------------------------
//  rtn_enter_burst
// ---------------------------------------------------------------------
void rtn_enter_burst(UINT32 routine_id, UINT16 call_site) {

    // forward all events to the cct driver so that the 
    // complete cct can be built
    #if COMPARE_TO_CCT == 1
    rtn_enter_cct(routine_id, call_site);
    #endif

    // push frame to shadow stack
    shadow_stack[curr_frame_idx].routine_id = routine_id;
    shadow_stack[curr_frame_idx].call_site  = call_site;
    curr_frame_idx++;
    
    // if burst is active, forward event to experiment driver
    // and increase event count
    if (bursting_on == YES) {
        #if ADAPTIVE == 1 && WEIGHT_COMPENSATION==1
        if (weight_comp_on == YES) {
            rtn_enter_wc(routine_id, call_site, RE_ENABLING);
            num_rtn_enter++;
        } else {
            //rtn_enter(routine_id, call_site);
            rtn_enter_wc(routine_id, call_site, 1);
            num_rtn_enter++;   
        }
        #else
        rtn_enter(routine_id, call_site);
        num_rtn_enter++;
        #endif
    }
    else if (bursting_on == MAYBE) {
        #if ADAPTIVE == 1
        burst_start(); // GESTIONE INTERNA
        #else
        // start a new burst
        burst_start();
        num_bursts++;
        bursting_on = YES;
        num_rtn_enter++; // ?? TODO
        #endif
    }
}


// ---------------------------------------------------------------------
//  rtn_exit_burst
// ---------------------------------------------------------------------
void rtn_exit_burst() {

    // forward all events to the cct driver so that the 
    // complete cct can be built
    #if COMPARE_TO_CCT == 1
    rtn_exit_cct();
    #endif

    // if burst is active, forward event to experiment driver
    // and increase event count
    if (bursting_on == YES) {
        rtn_exit();
        num_rtn_exit++;
    }

    // pop frame from shadow stack
    curr_frame_idx--;
}


// ---------------------------------------------------------------------
//  tick_burst
// ---------------------------------------------------------------------
void tick_burst() {

    // if we are at a sampling point, then schedule possible new burst 
    // (adaptive bursting may skip it).
    if (tick_counter % sampling_rate == 0)
        bursting_on = MAYBE;

    // when a burst expires, turn bursting off
    if (bursting_on == YES && tick_counter % burst_duration == 0) {
        bursting_on = NO;
        #if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
        weight_comp_on = NO;
        #endif
    }

    // increment master tick counter
    tick_counter++;
}


// ---------------------------------------------------------------------
//  init_burst
// ---------------------------------------------------------------------
// initialize bursting driver
int init_burst(int argc, char** argv) {

    int i;
    
    #if ADAPTIVE==1
    // default seed
    srand(0xABADCAFE);
    #endif
    
    // set defaults
    sampling_rate  = DEFAULT_SAMPLING_RATE;
    burst_duration = DEFAULT_BURST_DURATION;

    // scan command line arguments
    for (i = 0; i < argc; ++i) {

        // get sampling rate
        if (strcmp(argv[i], "-sampling") == 0) {
            if (i == argc-1) panic("missing argument of -sampling");
            sampling_rate = (UINT32)atoi(argv[i+1]);
            if (sampling_rate <= 0) panic("illegal -sampling argument");
        }

        // get burst duration
        if (strcmp(argv[i], "-burst") == 0) {
            if (i == argc-1) panic("missing argument of -burst");
            burst_duration = (UINT32)atoi(argv[i+1]);
            if (burst_duration <= 0) panic("illegal -burst argument");
        }
    }

    // make sure sampling rate is not smaller than burst duration
    if (sampling_rate < burst_duration)
        panic("sampling rate must be larger than burst duration");

    // make sure sampling rate is a multiple of burst duration
    if (sampling_rate % burst_duration != 0)
        panic("sampling rate must be a multiple of burst duration");

    // initialize shadow stack pointer
    curr_frame_idx = 0;

    // reset counters
    tick_counter = num_bursts = num_rtn_enter = num_rtn_exit = 0;

    // burst initially inactive
    bursting_on = NO;
    
    #if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
    weight_comp_on = NO;
    #endif

    // let specific test driver to initialize experiment
    return init(argc, argv);
}


// ---------------------------------------------------------------------
//  cleanup_burst
// ---------------------------------------------------------------------
// cleanup bursting driver
void cleanup_burst() {
    // CD101109: TO BE COMPLETED
    cleanup();
}


// ---------------------------------------------------------------------
//  write_output_burst
// ---------------------------------------------------------------------
void write_output_burst(logfile_t* f, driver_t* driver) {

    #if BURSTING == 1

    // save original stream length
    UINT64 full_stream_len = 
        driver->rtn_enter_events + driver->rtn_exit_events;

    // set driver counters with substream info
    driver->rtn_enter_burst_events = num_rtn_enter;
    driver->rtn_exit_burst_events  = num_rtn_exit;

    // let experiment driver log specific info
    write_output(f, driver);

    // write additional info about bursting
    logfile_add_to_header(f, "%-11s ", "samp. (us)");
    logfile_add_to_row   (f, "%-11lu ", 
        driver->timer_resolution * sampling_rate);

    logfile_add_to_header(f, "%-11s ", "burst (us)");
    logfile_add_to_row   (f, "%-11lu ", 
        driver->timer_resolution * burst_duration);

    logfile_add_to_header(f, "%-10s ", "num bursts");
    logfile_add_to_row   (f, "%-10llu ", num_bursts);

    logfile_add_to_header(f, "%-13s ", "tick res (us)");
    logfile_add_to_row   (f, "%-13lu ", driver->timer_resolution);

    logfile_add_to_header(f, "%-13s ", "app time (s)");
    logfile_add_to_row   (f, "%-13.2f ", 
        driver->timer_resolution * (tick_counter / 1000000.0));

    logfile_add_to_header(f, "%-10s ", "sampling %");
    logfile_add_to_row   (f, "%-10.2f ",
        100.0 * (num_rtn_enter + num_rtn_exit) / full_stream_len);

    #endif
}


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
