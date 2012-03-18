/* ============================================================================
 *  quicksorter.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 11, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 16:42:43 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __quicksorter__
#define __quicksorter__

#include "rlist.h"

// typdefs
typedef struct quicksorter_s quicksorter_t;

// ---------------------------------------------------------------------
// quicksorter_new
// ---------------------------------------------------------------------
/** create new sorter that maintains a sorted version of an input list
 *  the algorithm maintains a sorting network based on the problem
 *  decomposition made by the classical quicksort algorithm
 *  \param in address of variable holding head of input list 
 *  \param out address of variable holding head of output list
 *  \param map function that maps items of in to items of out
*/
quicksorter_t* quicksorter_new(rnode_t** in, rnode_t** out);


// ---------------------------------------------------------------------
// quicksorter_delete
// ---------------------------------------------------------------------
/** delete list quicksorter
 *  \param quicksorter quicksorter object 
*/
void quicksorter_delete(quicksorter_t* quicksorter);

#endif


/* Copyright (C) 2010 Camil Demetrescu

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
