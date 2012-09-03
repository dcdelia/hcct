/* ============================================================================
 *  joiner.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 11, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/11 09:26:32 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __joiner__
#define __joiner__

#include "rlist.h"

// typdefs
typedef struct joiner_s joiner_t;


// ---------------------------------------------------------------------
// joiner_new
// ---------------------------------------------------------------------
/** create new list joiner that concatenates
 *  two input lists into one new output lists - the input lists are
 *  not changed.
 *  \param in1 address of variable holding head of input list 1
 *  \param in2 address of variable holding head of input list 2
 *  \param out address of variable holding head of output list
*/
joiner_t* joiner_new(rnode_t** in1, rnode_t** in2, rnode_t** out);


// ---------------------------------------------------------------------
// joiner_delete
// ---------------------------------------------------------------------
/** delete list joiner
 *  \param joiner joiner object 
*/
void joiner_delete(joiner_t* joiner);

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