/* ============================================================================
 *  LScheduler.h
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        February 25, 2008
 *  Module:         ??

 *  Last changed:   $Date: 2010/11/02 17:55:21 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.2 $
*/


#ifndef __LScheduler__
#define __LScheduler__

#ifdef __cplusplus
extern "C" {
#endif
	
typedef char (*LScheduler_TComparator) (unsigned long inA, unsigned long inB);

void                    LScheduler_Init         (LScheduler_TComparator inComparator);
void                    LScheduler_Touched      (unsigned long cell_id, long int g_CurrCons);
unsigned long           LScheduler_Extract      ();
void                    LScheduler_CleanUp      ();

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
