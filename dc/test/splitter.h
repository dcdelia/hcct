/* ============================================================================
 *  splitter.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 6, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/14 13:14:47 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/


#ifndef __splitter__
#define __splitter__

#include "rlist.h"

// typdefs
typedef struct splitter_s splitter_t;


// ---------------------------------------------------------------------
// splitter_new
// ---------------------------------------------------------------------
/** create new list splitter that partitions the input list into
 * two sublists where all items are smaller/larger than the value of a
 * given pivot
 *  \param in address of variable holding head of input list 
 *  \param small address of variable holding head of output list 1
 *  \param large address of variable holding head of output list 2
 *  \param pivot address of variable holding pivot
*/
splitter_t* splitter_new(
    rnode_t** in, rnode_t** small, rnode_t** large, int* pivot);


// ---------------------------------------------------------------------
// splitter_delete
// ---------------------------------------------------------------------
/** delete list splitter
 *  \param splitter splitter object 
*/
void splitter_delete(splitter_t* splitter);

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
