/* =====================================================================
 *  analysis.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 9, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#ifndef __ANALYSIS__
#define __ANALYSIS__


#include "driver.h"
#include "cct.h"


#if COMPARE_TO_CCT==1
// ---------------------------------------------------------------------
//  finalPrune
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void finalPrune(cct_node_t *root, UINT64 N);
#endif


// ---------------------------------------------------------------------
//  rtn_enter
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void rtn_enter(UINT32 routine_id, UINT16 call_site);


#if ADAPTIVE==1
// ---------------------------------------------------------------------
//  rtn_enter_wc
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void rtn_enter_wc(UINT32 routine_id, UINT16 call_site,
                    UINT16 increment);
#endif


// ---------------------------------------------------------------------
//  rtn_exit
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void rtn_exit();


// ---------------------------------------------------------------------
//  tick
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void tick();


// ---------------------------------------------------------------------
//  init
// ---------------------------------------------------------------------
// to be defined by the analysis engine
int init(int argc, char** argv);


// ---------------------------------------------------------------------
//  cleanup
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void cleanup();


// ---------------------------------------------------------------------
//  write_output
// ---------------------------------------------------------------------
// to be defined by the analysis engine
void write_output(logfile_t* f, driver_t* d);


// ---------------------------------------------------------------------
//  burst_start
// ---------------------------------------------------------------------
// to be defined by the analysis engine
// called at the beginning of each burst
void burst_start();

#endif


/* Copyright (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

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
