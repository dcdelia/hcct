/* =====================================================================
 *  metrics.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/07 09:29:46 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1.1.1 $
*/


#ifndef __CCT_METRICS__
#define __CCT_METRICS__

#include "cct.h"
#include "analysis.h"

UINT32 nodes(cct_node_t* root);

UINT32 coldNodes(cct_node_t* root, int (*is_monitored)(cct_node_t* node));

UINT32 hotNodes(cct_node_t* root, UINT32 threshold, 
                int (*is_monitored)(cct_node_t* node));

UINT32 falsePositives(cct_node_t* root, UINT32 threshold, 
                      int (*is_monitored)(cct_node_t* node));

UINT64 sum(cct_node_t* root);

UINT32 hottest(cct_node_t* root);

UINT32 larger_than_hottest(cct_node_t* root, UINT32 TAU, UINT32 H2);

void uncoveredFrequency(cct_node_t* root, cct_node_t* cct_root,
                        UINT64* sum, UINT32* max, UINT32* uncovered);

void exactCounters(cct_node_t* root, cct_node_t* cct_root,
                   float *maxError, float *sumError,
                   UINT32 stream_threshold, int (*is_monitored)(cct_node_t* node),
                   UINT32 cct_threshold, UINT32 *falsePositives,
                   float adjustBursting);

void dump_max_phi(cct_node_t* root, logfile_t* f, driver_t* driver);

void dump_instance_data(cct_node_t* root, logfile_t* f, driver_t* driver);

void metrics_write_output(logfile_t* f, driver_t* driver,
                          UINT32 cct_threshold, 
                          UINT32 stream_threshold,
                          cct_node_t *hcct_root,
                          UINT32 epsilon, UINT32 phi, UINT32 tau,
                          int (*is_monitored)(cct_node_t* node));

#if COMPUTE_HOT_PER_PHI==1
UINT32 hotNodesCCT(cct_node_t* root, UINT32 threshold);

void dump_hot_per_phi(cct_node_t* cct_root, logfile_t* f, driver_t* driver);
#endif                          

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
