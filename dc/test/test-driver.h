/* ============================================================================
 *  test-driver.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 23, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/27 12:00:14 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.11 $
*/

#ifndef __testdriver__
#define __testdriver__

// default config parameters (to be overriden by makefile)

#ifndef CONV
#define CONV 0              // if 1, from-scratch conventional evaluation is tested
#endif

#ifndef WALL_CLOCK_TIME
#define WALL_CLOCK_TIME 1   // if 1, wall clock elapsed time is logged
#endif

#ifndef USER_SYS_TIME
#define USER_SYS_TIME   0   // if 1, user+system elapsed time is logged
#endif

#ifndef OPT
#define OPT "n/a"           // optimization flag "O0", "O3", etc. (defined by makefile)
#endif

#ifndef DC
#define DC 0                // if 1, allow dc-based programs to write additional info to log file
#endif

#ifndef CHECK
#define CHECK 1             // if 1, perform correctness checks
#endif

#ifndef MAKE_ASM_DUMP
#define MAKE_ASM_DUMP 0     // if 1 and DC == 1, write binary dump of dc app
                            // before and after code patching
#endif

#ifndef LOGS_DIR
#define LOGS_DIR "./"       // logs directory 
#endif


// include
#include <stdlib.h>


// typedefs
typedef struct test_s test_t;


// ---------------------------------------------------------------------
//  make_test
// ---------------------------------------------------------------------
// create new test object with input data of size input_size
// randomly generated with seed. If check_correctness != 0, then
// also perform (slow) correctness checks
test_t* make_test(
    size_t input_size, int seed, 
    const char* input_family, int check_correctness);


// ---------------------------------------------------------------------
//  del_test
// ---------------------------------------------------------------------
// free memory used by the test
void del_test(test_t* test);


// ---------------------------------------------------------------------
//  get_num_updates
// ---------------------------------------------------------------------
// return number of updates performed by the test
size_t get_num_updates(test_t* test);


// ---------------------------------------------------------------------
//  get_test_name
// ---------------------------------------------------------------------
// return test name
char* get_test_name();


// ---------------------------------------------------------------------
//  make_conv_input
// ---------------------------------------------------------------------
// prepare input for conventional evaluation
// return NULL if ok, error message otherwise
char* make_conv_input(test_t* test);


// ---------------------------------------------------------------------
//  do_conv_eval
// ---------------------------------------------------------------------
// conventional evaluation
// return NULL if ok, error message otherwise
char* do_conv_eval(test_t* test);


// ---------------------------------------------------------------------
//  make_updatable_input
// ---------------------------------------------------------------------
// construct updatable input for evaluation
// return NULL if ok, error message otherwise
char* make_updatable_input(test_t* test);


// ---------------------------------------------------------------------
//  do_from_scratch_eval
// ---------------------------------------------------------------------
// initial from-scratch evaluation of updatable input
// return NULL if ok, error message otherwise
char* do_from_scratch_eval(test_t* test);


// ---------------------------------------------------------------------
//  do_updates
// ---------------------------------------------------------------------
// do sequence of updates on updatable input
// return NULL if ok, error message otherwise
char* do_updates(test_t* test);


#endif


/* Copyright (C) 2010 Camil Demetrescu

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
