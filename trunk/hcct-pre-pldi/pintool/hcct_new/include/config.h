/* =====================================================================
 *  config.h
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2010/11/20 07:22:15 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.8 $
*/

#ifndef __CONFIG__
#define __CONFIG__


// config

#ifndef COMPARE_TO_CCT
#define COMPARE_TO_CCT          0
#endif

#ifndef COMPUTE_HOT_PER_PHI
#define COMPUTE_HOT_PER_PHI     0
#endif

#ifndef COMPUTE_MAX_PHI
#define COMPUTE_MAX_PHI         0
#endif

#ifndef BURSTING
#define BURSTING                0
#endif

#ifndef ADAPTIVE
#define ADAPTIVE                0
#endif

#ifndef RE_ENABLING
#define RE_ENABLING             0
#endif

#ifndef WEIGHT_COMPENSATION
#define WEIGHT_COMPENSATION     0
#endif

#ifndef LSS_ADJUST_EPS
#define LSS_ADJUST_EPS          0
#endif

#ifndef VERBOSE
#define VERBOSE                 0
#endif


// adjustable parameters

#define PAGE_SIZE               1024
#define STACK_MAX_DEPTH	        1024
#define DEFAULT_SAMPLING_RATE   50
#define DEFAULT_BURST_DURATION  1 
//#define DEFAULT_BUFFER_SIZE     (100*(1<<20))
#define DEFAULT_BUFFER_SIZE     (200*(1<<20))
#define DEFAULT_EPSILON         50000
#define DEFAULT_PHI             10000
#define DEFAULT_TAU             10


// typedefs
typedef unsigned short      UINT16;
typedef unsigned long       UINT32;
typedef unsigned long long  UINT64;

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
