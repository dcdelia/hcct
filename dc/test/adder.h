/* ============================================================================
 *  adder.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 10, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/10 14:01:22 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/


#ifndef __adder__
#define __adder__

#include "rlist.h"

// typdefs
typedef struct adder_s adder_t;


// ---------------------------------------------------------------------
// adder_new
// ---------------------------------------------------------------------
/** create new list adder that maintains the sum of items in a list
 *  \param in address of variable holding head of input list 
 *  \param sum address of variable holding the sum of all items
*/
adder_t* adder_new(rnode_t** in, int* sum);


// ---------------------------------------------------------------------
// adder_delete
// ---------------------------------------------------------------------
/** delete list adder
 *  \param adder adder object 
*/
void adder_delete(adder_t* adder);

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
