/* ============================================================================
 *  profile.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/28 15:38:42 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.5 $
*/


#ifndef __profile__
#define __profile__

#include <sys/times.h>
#include <sys/time.h>
#include <sys/param.h>


/* example of usage:

elapsed_time_t  elapsed_time;
time_rec_t start_time, end_time;

reset_elapsed_time(&elapsed_time);

repeat trials k times:

    get_time(&start_time);      // get initial time

    // task to measure...

    get_time(&end_time);        // get final time
    add_to_elapsed_time(&start_time, &end_time, &elapsed_time);

divide_elapsed_time_by(&elapsed_time, k);

size_t vm_peak = get_vm_peak();
*/

// typedef
typedef struct {
	struct timeval tv;
	struct tms tms;
} time_rec_t;

typedef struct {
    double real;
    double user;
    double system;
    double child_user;
    double child_system;
} elapsed_time_t;


// ---------------------------------------------------------------------
// get_time
// ---------------------------------------------------------------------
void get_time(time_rec_t* time_rec);


// ---------------------------------------------------------------------
//  reset_elapsed_time
// ---------------------------------------------------------------------
void reset_elapsed_time(elapsed_time_t* elapsed);


// ---------------------------------------------------------------------
//  divide_elapsed_time_by
// ---------------------------------------------------------------------
void divide_elapsed_time_by(elapsed_time_t* elapsed, double k);


// ---------------------------------------------------------------------
//  compute_elapsed_time
// ---------------------------------------------------------------------
void compute_elapsed_time(time_rec_t* since, time_rec_t* to, elapsed_time_t* elapsed);


// ---------------------------------------------------------------------
//  add_to_elapsed_time
// ---------------------------------------------------------------------
void add_to_elapsed_time(time_rec_t* since, time_rec_t* to, elapsed_time_t* elapsed);


// ---------------------------------------------------------------------
// dump_elapsed_time
// ---------------------------------------------------------------------
void dump_elapsed_time(time_rec_t* since, time_rec_t* to, char* msg, ...);


// ---------------------------------------------------------------------
// dump_proc_status
// ---------------------------------------------------------------------
void dump_proc_status(char* msg, ...);


// ---------------------------------------------------------------------
// get_vm_peak (in KB)
// ---------------------------------------------------------------------
int get_vm_peak();

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
