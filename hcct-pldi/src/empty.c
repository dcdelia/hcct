/* =====================================================================
 *  empty.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#include <stdio.h>
#include "driver.h"
#include "analysis.h"

#if ADAPTIVE==1
#include "burst.h"
#endif

#if ADAPTIVE==1
// ---------------------------------------------------------------------
//  ctxt_lookup
// ---------------------------------------------------------------------
int ctxt_lookup(UINT32 routine_id, UINT16 call_site) {
    return 1;
}
#endif


// ---------------------------------------------------------------------
//  burst_start
// ---------------------------------------------------------------------
void burst_start() {
    // resync for ADAPTIVE
    #if ADAPTIVE==1
    #if WEIGHT_COMPENSATION==1
    do_stack_walk(rtn_enter_wc, ctxt_lookup);
    #else
    do_stack_walk(rtn_enter, ctxt_lookup);
    #endif
    #endif
}


// ---------------------------------------------------------------------
//  rtn_enter
// ---------------------------------------------------------------------
void rtn_enter(UINT32 routine_id, UINT16 call_site) {
}

#if ADAPTIVE==1 && WEIGHT_COMPENSATION==1
// ---------------------------------------------------------------------
//  rtn_enter_wc
// ---------------------------------------------------------------------
void rtn_enter_wc(UINT32 routine_id,
                    UINT16 call_site, UINT16 increment) {
}
#endif


// ---------------------------------------------------------------------
//  rtn_exit
// ---------------------------------------------------------------------
void rtn_exit() {
}


// ---------------------------------------------------------------------
//  tick
// ---------------------------------------------------------------------
void tick() {
}


// ---------------------------------------------------------------------
//  init
// ---------------------------------------------------------------------
int init(int argc, char** argv) {
    return 0;
}


// ---------------------------------------------------------------------
//  cleanup
// ---------------------------------------------------------------------
void cleanup() {
}


// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
void write_output(logfile_t* f, driver_t* d) {
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
