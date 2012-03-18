/* =====================================================================
 *  driver.c
 * =====================================================================

 *  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
 *  License:        See the end of this file for license information
 *  Created:        November 4, 2010
 *  Module:         hcct

 *  Last changed:   $Date: 2011/08/19 16:13:14 $
 *  Changed by:     $Author: pctips $
 *  Revision:       $Revision: 1.5 $
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#include "analysis.h"
#include "driver.h"
#include "burst.h"


#ifndef O_BINARY
#define O_BINARY 0
#endif

// constants -- do not change
#define RTN_ENTER	            0x0
#define RTN_EXIT	            0x1
#define TIMER_TICK	            0x4
#define RTN_ENTER_CS            0x5
#define RTN_ENTER_LEN           5
#define RTN_ENTER_CS_LEN        7
#define MAX_EVENT_LEN           7
#define MAGIC_NUMBER            0xABADCAFE


#if BURSTING == 0
#define __target(x) x
#else 
#define __target(x) x##_burst
#endif


// prototypes
driver_t* driver_open(int argc, char** argv, size_t buf_size);
void driver_read(driver_t* driver);
void driver_close(driver_t* driver);


// ---------------------------------------------------------------------
// print_msg
// ---------------------------------------------------------------------
static void print_msg(FILE* fp, char* msg, va_list args) {
    fprintf(fp, "[driver] ");
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
void panic(char* msg, ...) {
    va_list args;
    va_start(args, msg);
    print_msg(stderr, msg, args);
    va_end(args);
    exit(1);
}


// ---------------------------------------------------------------------
//  analyze_buffer
// ---------------------------------------------------------------------
//~ static size_t analyze_buffer2(char* buf, size_t buf_size) {
//~ 
    //~ size_t left;
    //~ char *limit;
    //~ 
    //~ message("Buf_size: %u", buf_size);
//~ 
    //~ if (buf_size > MAX_EVENT_LEN)
        //~ //limit = buf + buf_size - MAX_EVENT_LEN;
        //~ limit = buf + buf_size - MAX_EVENT_LEN + 1;
    //~ else
        //~ // TODO: check +MAX_EVENT_LEN insted of +buf_size??
        //~ limit = buf + buf_size; 
//~ 
    //~ // scan buffer
    //~ while (buf < limit)
        //~ switch (*buf) {
            //~ case RTN_ENTER_CS:
                //~ __target(rtn_enter)(*(UINT32*)(buf+1), *(UINT16*)(buf+5));
                //~ buf += RTN_ENTER_CS_LEN;
                //~ break;
//~ 
            //~ case RTN_EXIT:
                //~ __target(rtn_exit)();
                //~ ++buf;
                //~ break;
//~ 
            //~ case RTN_ENTER:
                //~ __target(rtn_enter)(*(UINT32*)(buf+1), 0);
                //~ buf += RTN_ENTER_LEN;
                //~ break;
//~ 
            //~ case TIMER_TICK:
                //~ #if BURSTING == 1
                //~ __target(tick)();
                //~ #endif
                //~ ++buf;
                //~ break;
//~ 
            //~ default: panic("illegal stream event");
        //~ }
    //~ 
    //~ if (buf_size == MAX_EVENT_LEN) return 0;
    //~ 
    //~ //left = (size_t)((limit + MAX_EVENT_LEN)-buf); // TODO: check this!
    //~ left = (size_t)((limit + (MAX_EVENT_LEN-1))-buf); // TODO: check this!
    //~ message("left: %u", left);
//~ 
    //~ if (left > 0)
        //~ memcpy((limit + MAX_EVENT_LEN - buf_size), buf, left);
    //~ 
    //~ return left;
//~ }

static size_t analyze_buffer(char* buf, size_t buf_size) {
    
    //message("Buf_size: %u", buf_size);

    char* limit = buf + (buf_size-1);
    size_t left=0;
    int scan=1;

    // scan buffer
    while (scan && buf<=limit)
        switch (*buf) {
            case RTN_ENTER_CS:
                if (limit-buf >= RTN_ENTER_CS_LEN-1) {
                    __target(rtn_enter)(*(UINT32*)(buf+1), *(UINT16*)(buf+5));
                    buf += RTN_ENTER_CS_LEN;
                } else {
                    left=1+(limit-buf); // CHECK occhio!
                    scan=0;
                }
                break;
                
            case RTN_ENTER:
                if (limit-buf >= RTN_ENTER_LEN-1) {
                    __target(rtn_enter)(*(UINT32*)(buf+1), 0);
                    buf += RTN_ENTER_LEN;
                } else {
                    left=1+(limit-buf); // CHECK occhio!
                    scan=0;
                }
                break;

            case RTN_EXIT:
                __target(rtn_exit)();
                if (limit==buf) scan=0;
                ++buf;
                break;

            case TIMER_TICK:
                #if BURSTING == 1
                __target(tick)();
                #endif
                if (limit==buf) scan=0;
                ++buf;
                break;

            default: panic("illegal stream event");
        }
    
    //message("Left: %u", left);
    
    if (left==0) return 0;
    else {
        // buf starts at limit+1-buf_size
        memcpy(((limit+1)-buf_size), buf, left);
        return left;
    }
}



// ---------------------------------------------------------------------
//  driver_open
// ---------------------------------------------------------------------
driver_t* driver_open(int argc, char** argv, size_t buf_size) {

    int ret;
    
    // allocate driver object
    driver_t* driver = (driver_t*)malloc(sizeof(driver_t));
    if (driver == NULL) return NULL;
    
    // allocate buffer
    driver->buffer = (char*)malloc(buf_size);
    if (driver->buffer == NULL) {
        free(driver);
        return NULL;
    }

    // keep track of buffer size and in/out file names
    driver->buf_size   = buf_size;
    driver->trace_file = argv[1];
    driver->benchmark  = argv[2];
    driver->log_file   = argv[3];

    // open file in large file mode
    driver->fd = open(driver->trace_file, O_RDONLY|O_BINARY|O_LARGEFILE);
    if (driver->fd == -1) {
        free(driver->buffer);
        free(driver);
        return NULL;
    }

    // read file header
    ret = read(driver->fd, driver->buffer, 40); // see TraceWriter.cpp
    if (ret==-1) {
        driver_close(driver);
        return NULL;
    }

    // check file magic number
    if (*(UINT32*)driver->buffer != MAGIC_NUMBER) {
        driver_close(driver);
        return NULL;
    }
    
    // parse file header
    driver->rtn_enter_events=*(UINT64*)(driver->buffer+4);
    driver->rtn_exit_events=*(UINT64*)(driver->buffer+12);
    driver->ticks=*(UINT64*)(driver->buffer+20);
    driver->first_tick=*(UINT64*)(driver->buffer+28);
    driver->timer_resolution=*(UINT32*)(driver->buffer+36);

    // init time
    reset_elapsed_time(&driver->elapsed_time);
    driver->iterations=0;
    
    return driver;
}


// ---------------------------------------------------------------------
//  driver_run_experiment
// ---------------------------------------------------------------------
int driver_run_experiment(driver_t* driver) {

    int         ret; 
    int         left = 0;
    time_rec_t  start_time;
    time_rec_t  end_time;

    // scanning loop
    for (;;) {

        //~ message("starting iteration #%d", driver->iterations + 1);
        //~ message("   reading buffer...");

        // load buffer
        ret = read(
            driver->fd, driver->buffer+left, driver->buf_size-left);
        if (ret == -1) return -1;
        if (ret == 0) break; // && left!=0 non è necessario
        
        // sleep???
        //usleep(1000);

        message("starting iteration #%d", driver->iterations + 1);
        message("   analyzing buffer...");

        ++driver->iterations;

        // take time before analyzing the buffer
        //get_time(&start_time);
        
        // Privilegio WC ???
        times(&start_time.tms);
	    gettimeofday(&start_time.tv, NULL);

        // analyze buffer
        left = analyze_buffer(driver->buffer, ret + left);

        // take time after analyzing the buffer
        //get_time(&end_time);
        
        // Privilegio WC ???
	    gettimeofday(&end_time.tv, NULL);
	    times(&end_time.tms);

        // cumulate iteration time
        add_to_elapsed_time(&start_time, &end_time, 
                            &driver->elapsed_time);    
        
        message("   buffer analyzed.");
    }

    return 0;
}


// ---------------------------------------------------------------------
//  driver_write_output
// ---------------------------------------------------------------------
void driver_write_output(driver_t* driver) {

    #if COMPUTE_MAX_PHI == 1

    // open log file in overwrite mode
    logfile_t* f = logfile_open(driver->log_file, 0);
    if (f == NULL)
        panic("can't open log file %s", driver->log_file);    
    
    #else

    // open log file in append mode
    logfile_t* f = logfile_open(driver->log_file, 1);
    if (f == NULL)
        panic("can't open log file %s", driver->log_file);    

    // header starts with # so that it can be skipped by gnuplot
    logfile_add_to_header(f, "%-14s ", "#benchmark");
    logfile_add_to_row   (f, "%-14s ", driver->benchmark);

    #if COMPARE_TO_CCT == 1

    logfile_add_to_header(f, "%-11s ", "N");
    logfile_add_to_row   (f, "%-11llu ", driver->rtn_enter_events);

    #else

    logfile_add_to_header(f, "%-11s ", "num enter");
    logfile_add_to_row   (f, "%-11llu ", 
        driver->rtn_enter_events);

    logfile_add_to_header(f, "%-11s ", "num exit");
    logfile_add_to_row   (f, "%-11llu ", 
        driver->rtn_exit_events);

    logfile_add_to_header(f, "%-16s ", "tot su time (s)");
    logfile_add_to_row   (f, "%-16.3f ", 
        driver->elapsed_time.user + driver->elapsed_time.system);

    logfile_add_to_header(f, "%-19s ", "avg op su time (ns)");
    logfile_add_to_row   (f, "%-19.3f ", 1000000000.0 *
        ((driver->elapsed_time.user + driver->elapsed_time.system)/
         (driver->rtn_enter_events + driver->rtn_exit_events)));

    logfile_add_to_header(f, "%-16s ", "tot wc time (s)");
    logfile_add_to_row   (f, "%-16.6f ", 
        driver->elapsed_time.real);

    logfile_add_to_header(f, "%-19s ", "avg op wc time (ns)");
    logfile_add_to_row   (f, "%-19.6f ", 1000000000.0 *
        (driver->elapsed_time.real/
         (driver->rtn_enter_events + driver->rtn_exit_events)));

    logfile_add_to_header(f, "%-14s ", "memory (KB)");
    logfile_add_to_row   (f, "%-14lu ", driver->memory);

    #endif
    #endif

    // allow analyzer to log specific data
    __target(write_output)(f, driver);

    // close log file
    logfile_close(f);
}


// ---------------------------------------------------------------------
//  driver_close
// ---------------------------------------------------------------------
void driver_close(driver_t* driver) {
    close(driver->fd);
    free(driver->buffer);
    free(driver);
}


// ---------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------
int main(int argc, char** argv) {

    // check command line parameters
    if (argc < 4)
        panic("usage: %s trace_file benchmark log_file [params]", 
            argv[0]);

    // create driver for trace file
    driver_t* driver = 
        driver_open(argc, argv, DEFAULT_BUFFER_SIZE);
    if (driver == NULL) panic("cannot init driver");

    // dump file information
    fprintf(stderr, "--------------------> Command line <--------------------\n");
    int a;
    for (a=0; a<argc; ++a)
        fprintf(stderr, "%s ", argv[a]);
    fprintf(stderr,"\n");
    fprintf(stderr, "--------------------------------------------------------\n");
    
    
    message("trace file name: %s", argv[1]);
    message("routine enter events: %llu", driver->rtn_enter_events);
    message("routine exit events: %llu", driver->rtn_exit_events);
    message("total number of ticks: %llu", driver->ticks);
    message("first tick since application startup: %llu", 
        driver->first_tick);
    message("timer resolution (in microseconds): %lu", 
        driver->timer_resolution);
    message("buffer size: %lu MB", DEFAULT_BUFFER_SIZE/(1<<20));
    message("starting analysis of recorded trace...");

    // get base memory
    size_t base_memory = get_vm_peak();

    // init analyzer
    if (__target(init)(argc, argv) == -1) 
        panic("cannot initialize analyzer");

    // run experiment
    driver_run_experiment(driver);

    // compute memory required by the analyzer
    driver->memory = get_vm_peak() - base_memory;

    // log experimental results
    driver_write_output(driver);

    // cleanup experiment
    __target(cleanup)();

    // close trace file
    driver_close(driver);

    return 0;
}


/* Copyright (C) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
*/

