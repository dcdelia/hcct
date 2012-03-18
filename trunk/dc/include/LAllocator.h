/* ============================================================================
 *  LAllocator.h
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        February 7, 2008
 *  Module:         ??

 *  Last changed:   $Date: 2010/09/15 08:42:03 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.1.1.1 $
*/


#ifndef __LAllocator__
#define __LAllocator__

#ifdef __cplusplus
extern "C" {
#endif

void                    LAllocator_Init         (unsigned long record_size, void **ptr);
unsigned long int       LAllocator_Alloc        (void **ptr);
void                    LAllocator_Free         (unsigned long int index, void **ptr);
void                    LAllocator_CleanUp      (void **ptr);

unsigned long int       LAllocator_GetN         (void **ptr);
unsigned long int       LAllocator_GetH         (void **ptr);

#ifdef __cplusplus
}
#endif

#endif


/* Copyright (C) 2008 Camil Demetrescu, Andrea Ribichini

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
