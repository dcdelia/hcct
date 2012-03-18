/* =====================================================================
 *  cct.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/

#ifndef __CCT__
#define __CCT__

#include "driver.h"

// cct node structure: 18+2 bytes (on 32 bit architectures)
typedef struct cct_node_s cct_node_t;
struct cct_node_s {
    UINT32      routine_id;    
    UINT32      counter;
    cct_node_t* first_child;
    cct_node_t* next_sibling;
    UINT16      call_site;
};

void cctPrune(cct_node_t* root, UINT32 threshold);

#if COMPARE_TO_CCT==1
void burst_start_cct();
void rtn_enter_cct(UINT32 routine_id, UINT16 call_site);
void rtn_exit_cct();
int init_cct(int argc, char** argv);
void cleanup_cct();
void write_output_cct(logfile_t* f, driver_t* d);

#if ADAPTIVE==1 // TODO spostare altrove!!!
//~ cct_node_t* hcct_get_root();

#if WEIGHT_COMPENSATION==1
void rtn_enter_wc_cct(UINT32 routine_id, UINT16 call_site,
                        UINT16 increment);
#endif

#endif

#endif

// ---------------------------------------------------------------------
//  cct_get_root
// ---------------------------------------------------------------------
cct_node_t* cct_get_root();

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
