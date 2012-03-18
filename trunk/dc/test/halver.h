/* ============================================================================
 *  halver.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 27, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/27 16:16:33 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __halver__
#define __halver__

#include "rlist.h"

// typdefs
typedef struct halver_s halver_t;


// ---------------------------------------------------------------------
// halver_new
// ---------------------------------------------------------------------
/** create new randomized list halver that maintains a balanced partition 
 *  of an input list into two output lists
 *  \param in address of variable holding head of input list 
 *  \param out1 address of variable holding head of output list 1
 *  \param out2 address of variable holding head of output list 2
*/
halver_t* halver_new(rnode_t** in, rnode_t** out1, rnode_t** out2);


// ---------------------------------------------------------------------
// halver_delete
// ---------------------------------------------------------------------
/** delete list halver
 *  \param halver halver object 
*/
void halver_delete(halver_t* halver);

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
