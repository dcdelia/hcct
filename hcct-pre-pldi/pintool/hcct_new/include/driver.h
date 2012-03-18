/* =====================================================================
 *  driver.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/13 10:43:08 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/


#ifndef __DRIVER__
#define __DRIVER__

#include "config.h"
#include "profile.h"
#include "logfile.h"


// driver structure
typedef struct driver_s driver_t;
struct driver_s {
    int             fd;
    char*           trace_file;
    char*           log_file;
    char*           benchmark;
    size_t          iterations;
    char*           buffer;
    size_t          buf_size;
    UINT32          timer_resolution;

                    // count of all events in the trace
    UINT64          rtn_enter_events, rtn_exit_events, ticks, first_tick;

    #if BURSTING == 1
                    // count of actual events processed during bursts
    UINT64          rtn_enter_burst_events, rtn_exit_burst_events;
    #endif

    elapsed_time_t  elapsed_time;

    size_t          memory;
};


// ---------------------------------------------------------------------
//  panic
// ---------------------------------------------------------------------
void panic(char* msg, ...);

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
