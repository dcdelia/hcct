/* ============================================================================
 *  test-driver.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 23, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/18 21:47:05 $
 *  Changed by:     $Author: ribbi $
 *  Revision:       $Revision: 1.23 $
*/


#include <stdarg.h>
#include <stdio.h>

#include "logfile.h"
#include "test-driver.h"
#include "profile.h"

#if DC
#include "rm.h"
#include "dc.h"
#endif


// config
#define DUMP_PROC_STATUS 0


// driver structure
typedef struct {
    size_t          input_size;
    size_t          num_updates;
    char*           log_file_name;
    char*           test_name;
    char*           input_family;
    int             seed;
    size_t          trials;
    test_t*         test;
    int             vm_peak;
    elapsed_time_t  input_time;
    elapsed_time_t  init_time;
    elapsed_time_t  update_time;

    #if CONV
    elapsed_time_t  conv_input_time;
    elapsed_time_t  conv_eval_time;
    #endif

    #if DC
    size_t          init_exec_cached;   // number of cached instruction 
                                        // executed during from-scratch evaluation
    size_t          update_exec_cached; // number of cached instructions
                                        // executed during update sequence
    size_t          init_num_cons;      // number of constraints after from-scratch ev.
    size_t          update_num_cons;    // number of constraints after update sequence
    size_t          init_exec_cons;     // number of cons. exec. during from-scr. ev.
    size_t          update_exec_cons;   // number of cons. exec. during update sequence
    #endif
} driver_t;


// ---------------------------------------------------------------------
// print_msg
// ---------------------------------------------------------------------
static void print_msg(FILE* fp, char* msg, va_list args) {
    fprintf(fp, "[test-driver] ");
	vfprintf(fp, msg, args);
    fprintf(fp, "\n");
}


// ---------------------------------------------------------------------
// message
// ---------------------------------------------------------------------
static void message(char* msg, ...) {
    va_list args;
    va_start(args, msg);
    print_msg(stderr, msg, args);
    va_end(args);
}


// ---------------------------------------------------------------------
// panic
// ---------------------------------------------------------------------
static void panic(char* msg, ...) {
    va_list args;
    va_start(args, msg);
    print_msg(stderr, msg, args);
    va_end(args);
    exit(1);
}

#if DC && MAKE_ASM_DUMP
void do_make_binary_dump(driver_t* driver, char* suffix) {
    char buf[128];
    rfree(rmalloc(8));
    sprintf(buf, "%s%s-%s%s.dump", LOGS_DIR, driver->test_name, OPT, suffix);
    rm_make_dump_file(buf);
}
#endif

// ---------------------------------------------------------------------
// do_trials
// ---------------------------------------------------------------------
void do_trials(driver_t* driver) {

    time_rec_t start_time, end_time;
    int k;
    char* res;

    // init time counters
    reset_elapsed_time(&driver->input_time);
    reset_elapsed_time(&driver->init_time);
    reset_elapsed_time(&driver->update_time);
    #if CONV
    reset_elapsed_time(&driver->conv_input_time);
    reset_elapsed_time(&driver->conv_eval_time);
    #endif

    // make trials
    for (k = 1; k <= driver->trials; ++k) {

        // make test object
        driver->test = make_test(driver->input_size, driver->seed, driver->input_family, 0);
        if (driver->test == NULL) panic("cannot create test object");

        // starting test
        message("starting trial #%d: %s, input size=%u, input family=%s, seed=%d, opt=%s", 
            k, driver->test_name, driver->input_size, driver->input_family, driver->seed, OPT);

        // make updatable input for evaluation
        message("make updatable input...");
        get_time(&start_time);      // get initial time
        res = make_updatable_input(driver->test);
        if (res != NULL) panic(res);
        get_time(&end_time);        // get final time
        add_to_elapsed_time(&start_time, &end_time, &driver->input_time);
        message("...done.");
    
        // initial from-scratch evaluation of updatable input
        message("perform from-scratch evaluation...");
        #if DC
        if (k==1) {
            driver->init_exec_cached = g_stats.exec_cache_instr_count;
            driver->init_exec_cons = dc_num_exec_cons();
        }
        #endif
        get_time(&start_time);      // get initial time
        res = do_from_scratch_eval(driver->test);
        if (res != NULL) panic(res);
        get_time(&end_time);        // get final time
        add_to_elapsed_time(&start_time, &end_time, &driver->init_time);
        #if DC
        if (k==1) {
            driver->init_exec_cached = 
                g_stats.exec_cache_instr_count - driver->init_exec_cached;
            driver->init_exec_cons = 
                dc_num_exec_cons() - driver->init_exec_cons;
            driver->init_num_cons = dc_num_cons();
        }
        #endif
        message("...done.");

        // do sequence of updates on updatable input
        message("perform update sequence...");
        #if DC
        if (k==1) {
            driver->update_exec_cached = g_stats.exec_cache_instr_count;
            driver->update_exec_cons = dc_num_exec_cons();
        }
        #endif
        get_time(&start_time);      // get initial time
        res = do_updates(driver->test);
        if (res != NULL) panic(res);
        get_time(&end_time);        // get final time
        add_to_elapsed_time(&start_time, &end_time, &driver->update_time);
        #if DC
        if (k==1) {
            driver->update_exec_cached = 
                g_stats.exec_cache_instr_count - driver->update_exec_cached;
            driver->update_exec_cons = 
                dc_num_exec_cons() - driver->update_exec_cons;
            driver->update_num_cons = dc_num_cons();
        }
        #endif
        message("...done.");

        // get peak memory used by process after first trial
        // (ignoring conventional evaluation)
        if (k==1) driver->vm_peak = get_vm_peak();

        // fetch actual number of updates performed
        driver->num_updates = get_num_updates(driver->test);

        #if CONV
        // make conventional input for evaluation
        message("make conventional input...");
        get_time(&start_time);      // get initial time
        res = make_conv_input(driver->test);
        if (res != NULL) panic(res);
        get_time(&end_time);        // get final time
        add_to_elapsed_time(&start_time, &end_time, &driver->conv_input_time);
    
        // conventional evaluation
        message("perform conventional evaluation...");
        get_time(&start_time);      // get initial time
        res = do_conv_eval(driver->test);
        if (res != NULL) panic(res);
        get_time(&end_time);        // get final time
        add_to_elapsed_time(&start_time, &end_time, &driver->conv_eval_time);
        #endif

        // deallocate test object
        del_test(driver->test);
    }

    // compute average time per trial
    divide_elapsed_time_by(&driver->input_time,  driver->trials);
    divide_elapsed_time_by(&driver->init_time,   driver->trials);
    divide_elapsed_time_by(&driver->update_time, driver->trials);
    #if CONV
    divide_elapsed_time_by(&driver->conv_input_time, driver->trials);
    divide_elapsed_time_by(&driver->conv_eval_time, driver->trials);
    #endif
}


// ---------------------------------------------------------------------
// do_checks
// ---------------------------------------------------------------------
void do_checks(driver_t* driver) {

    // perform final checks
    #if CHECK

    char* res;

    // make test object
    driver->test = make_test(driver->input_size, driver->seed, driver->input_family, 1);
    if (driver->test == NULL) panic("cannot create test object");

    // starting correctness check    
    message("checking correctness: %s, input size=%u, input family=%s seed=%d opt=%s", 
        driver->test_name, driver->input_size, driver->input_family, driver->seed, OPT);

    // check updatable input construction
    message("check updatable input construction...");
    res = make_updatable_input(driver->test);
    if (res != NULL) panic("...failed (%s)", res);
    else message("...passed");

    // check initial from-scratch evaluation of updatable input
    message("check from-scratch evaluation...");
    res = do_from_scratch_eval(driver->test);
    if (res != NULL) panic("...failed (%s)", res);
    else message("...passed");

    // do sequence of updates on updatable input
    message("check update sequence...");
    res = do_updates(driver->test);
    if (res != NULL) panic("...failed (%s)", res);
    else message("...passed");

    // deallocate test object
    del_test(driver->test);

    #endif
}


// ---------------------------------------------------------------------
// do_write_output
// ---------------------------------------------------------------------
void do_write_output(driver_t* driver) {

    logfile_t* f = logfile_open(driver->log_file_name, 1);
    if (f == NULL)
        panic("can't open log file %s", driver->log_file_name);    

    logfile_add_to_header(f, "%-14s ", "test name");
    logfile_add_to_row   (f, "%-14s ", driver->test_name);

    logfile_add_to_header(f, "%-11s ", "input size");
    logfile_add_to_row   (f, "%-11u ", driver->input_size);

    logfile_add_to_header(f, "%-14s ", "input family");
    logfile_add_to_row   (f, "%-14s ", driver->input_family);

    logfile_add_to_header(f, "%-12s ", "num updates");
    logfile_add_to_row   (f, "%-12u ", driver->num_updates);

    logfile_add_to_header(f, "%-8s ", "seed");
    logfile_add_to_row   (f, "%-8d ", driver->seed);

    logfile_add_to_header(f, "%-4s ", "opt");
    logfile_add_to_row   (f, "%-4s ", OPT);

    logfile_add_to_header(f, "%-7s ", "trials");
    logfile_add_to_row   (f, "%-7d ", driver->trials);

    #if WALL_CLOCK_TIME
    logfile_add_to_header(f, "%-16s ",   "input real (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->input_time.real);

    logfile_add_to_header(f, "%-16s ",   "init real (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->init_time.real);

    logfile_add_to_header(f, "%-16s ",   "update real (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->update_time.real);
    #endif

    #if USER_SYS_TIME
    logfile_add_to_header(f, "%-16s ",   "input u+s (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->input_time.user + driver->input_time.system);

    logfile_add_to_header(f, "%-16s ",   "init u+s (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->init_time.user + driver->init_time.system);

    logfile_add_to_header(f, "%-16s ",   "update u+s (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->update_time.user + driver->update_time.system);
    #endif

    #if CONV
    
    #if WALL_CLOCK_TIME
    logfile_add_to_header(f, "%-16s ",   "con inp re (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->conv_input_time.real);

    logfile_add_to_header(f, "%-16s ",   "con eva re (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->conv_eval_time.real);
    #endif
    
    #if USER_SYS_TIME
    logfile_add_to_header(f, "%-16s ",   "con inp u+s (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->conv_input_time.user + driver->conv_input_time.system);

    logfile_add_to_header(f, "%-16s ",   "con eva u+s (s)");
    logfile_add_to_row   (f, "%-16.3f ", driver->conv_eval_time.user + driver->conv_eval_time.system);
    #endif
    
    #endif

    logfile_add_to_header(f, "%-13s ", "vm peak (KB)");
    logfile_add_to_row   (f, "%-13d ", driver->vm_peak);

    #if DC
    logfile_add_to_header(f, "%-13s ", "init cons");
    logfile_add_to_row   (f, "%-13u ", driver->init_num_cons);

    logfile_add_to_header(f, "%-13s ", "upd cons");
    logfile_add_to_row   (f, "%-13u ", driver->update_num_cons);

    logfile_add_to_header(f, "%-13s ", "init ex cons");
    logfile_add_to_row   (f, "%-13u ", driver->init_exec_cons);

    logfile_add_to_header(f, "%-13s ", "upd ex cons");
    logfile_add_to_row   (f, "%-13u ", driver->update_exec_cons);

    logfile_add_to_header(f, "%-13s ", "avg con x upd");
    logfile_add_to_row   (f, "%-13.1f ", (double)driver->update_exec_cons/driver->num_updates);

    logfile_add_to_header(f, "%-14s ", "init ex cached");
    logfile_add_to_row   (f, "%-14u ", driver->init_exec_cached);

    logfile_add_to_header(f, "%-13s ", "upd ex cached");
    logfile_add_to_row   (f, "%-13u ", driver->update_exec_cached);
    #endif

    logfile_close(f);
}


// ---------------------------------------------------------------------
// main
// ---------------------------------------------------------------------
int main(int argc, char** argv) {

    driver_t d, *driver = &d;

    if (argc < 6)
        panic("usage: %s inputsize inputfamily logfile seed trials\n", argv[0]);

    // get command line parameters
    driver->input_size = atol(argv[1]);
    if (driver->input_size == 0) panic("illegal size parameter");
    driver->input_family = argv[2];
    driver->log_file_name = argv[3];
    driver->seed = atol(argv[4]);
    if (driver->seed == 0) panic("illegal seed parameter");
    driver->trials = atol(argv[5]);
    if (driver->trials == 0) panic("illegal trials parameter");
    driver->test_name = get_test_name();

    // make dump of unpatched dc binary
    #if DC && MAKE_ASM_DUMP
    do_make_binary_dump(driver, "-start");
    #endif

    // do performance test
    do_trials(driver);

    // do correctness test
    do_checks(driver);

    // write results to log file
    do_write_output(driver);
    
    /*printf ("--mem_read_count = %u\n", dc_stat.mem_read_count);
    printf ("--first_mem_read_count = %u\n", dc_stat.first_mem_read_count);
    printf ("--mem_write_count = %u\n", dc_stat.mem_write_count);
    printf ("--first_mem_write_count = %u\n", dc_stat.first_mem_write_count);
    printf ("--case_rw_count = %u\n", dc_stat.case_rw_count);
    printf ("--case_rw_first_count = %u\n", dc_stat.case_rw_first_count);*/
    
    // make dump of patched dc binary
    #if DC && MAKE_ASM_DUMP
    do_make_binary_dump(driver, "-end");
    #endif

    // dump final process status
    #if DUMP_PROC_STATUS
    dump_proc_status("final process status");
    #endif

    #if DC
    printf ("# of patchable instr: %u\n", g_stats.patch_instr_number);
    //printf ("# of AVs: %u\n", g_num_av);
    #endif

    return 0;
}


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
