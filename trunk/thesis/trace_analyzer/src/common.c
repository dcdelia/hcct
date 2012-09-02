/* =====================================================================
 *  common.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 9, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/03/12 20:08:37 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.2 $
*/

#include "common.h"
#include <string.h>
#include <stdlib.h>


// ---------------------------------------------------------------------
//  get_epsilon_phi_tau
// ---------------------------------------------------------------------
void get_epsilon_phi_tau(int argc, char** argv, 
                         UINT32 *epsilon, UINT32 *phi, UINT32 *tau) {

    int i;

    // set defaults
    *phi     = DEFAULT_PHI;
    *epsilon = DEFAULT_EPSILON;
    *tau     = DEFAULT_TAU;

    // scan command line arguments
    for (i = 0; i < argc; ++i) {

        // get 1/phi
        if (strcmp(argv[i], "-phi") == 0) {
            if (i == argc-1) panic("missing argument of -phi");
            *phi = (UINT32)strtol(argv[i+1], NULL, 0);
            if (*phi <= 0) panic("illegal -phi argument");
        }

        // get 1/epsilon
        if (strcmp(argv[i], "-epsilon") == 0) {
            if (i == argc-1) panic("missing argument of -epsilon");
            *epsilon = (UINT32)strtol(argv[i+1], NULL, 0);
            if (*epsilon <= 0) panic("illegal -epsilon argument");
        }

        // get 1/tau
        if (strcmp(argv[i], "-tau") == 0) {
            if (i == argc-1) panic("missing argument of -tau");
            *tau = strtol(argv[i+1], NULL, 0);
            if (*tau <= 0) panic("illegal -tau argument");
        }
    }

    // check parameters
    if (*phi > *epsilon) panic("it must be epsilon >= phi");
}


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
