/* ============================================================================
 *  profile.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/24 14:56:05 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/

#include "profile.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// ---------------------------------------------------------------------
// get_time
// ---------------------------------------------------------------------
void get_time(time_rec_t* time_rec){
	gettimeofday(&time_rec->tv, NULL);
	times(&time_rec->tms);
}



// ---------------------------------------------------------------------
//  reset_elapsed_time
// ---------------------------------------------------------------------
void reset_elapsed_time(elapsed_time_t* elapsed) {
    elapsed->real         = 0;
    elapsed->user         = 0;
    elapsed->system       = 0;
    elapsed->child_user   = 0;
    elapsed->child_system = 0;
}


// ---------------------------------------------------------------------
//  divide_elapsed_time_by
// ---------------------------------------------------------------------
void divide_elapsed_time_by(elapsed_time_t* elapsed, double k) {
    elapsed->real         /= k;
    elapsed->user         /= k;
    elapsed->system       /= k;
    elapsed->child_user   /= k;
    elapsed->child_system /= k;
}


// ---------------------------------------------------------------------
//  add_to_elapsed_time
// ---------------------------------------------------------------------
void add_to_elapsed_time(time_rec_t* since, time_rec_t* to, elapsed_time_t* elapsed){
    elapsed->real   += to->tv.tv_sec+(to->tv.tv_usec*0.000001) -
                       since->tv.tv_sec-(since->tv.tv_usec*0.000001);
    elapsed->user   += (double)((to->tms.tms_utime)-(since->tms.tms_utime))/HZ;
    elapsed->system += (double)((to->tms.tms_stime)-(since->tms.tms_stime))/HZ;
    elapsed->child_user += (double)((to->tms.tms_cutime)-(since->tms.tms_cutime))/HZ;
    elapsed->child_system += (double)((to->tms.tms_cstime)-(since->tms.tms_cstime))/HZ;
}


// ---------------------------------------------------------------------
//  compute_elapsed_time
// ---------------------------------------------------------------------
void compute_elapsed_time(time_rec_t* since, time_rec_t* to, elapsed_time_t* elapsed) {
    reset_elapsed_time(elapsed);
    add_to_elapsed_time(since, to, elapsed);
}


// ---------------------------------------------------------------------
// dump_elapsed_time
// ---------------------------------------------------------------------
void dump_elapsed_time(time_rec_t* since, time_rec_t* to, char* msg, ...){
    va_list args;
    elapsed_time_t elapsed;
    
    compute_elapsed_time(since, to, &elapsed);

    va_start(args, msg);
	printf ("[profile] ");
	vprintf(msg, args);
	printf ("\n[profile] - Real time: %f sec\n", elapsed.real);
	printf ("[profile] - User time: %f sec\n", elapsed.user);
	printf ("[profile] - System time: %f sec\n", elapsed.system);
	printf ("[profile] - User time (children): %f sec\n", elapsed.child_user);
	printf ("[profile] - System time (children): %f sec\n", elapsed.child_system);
    va_end(args);
}


// ---------------------------------------------------------------------
// dump_proc_status
// ---------------------------------------------------------------------
void dump_proc_status(char* msg, ...) {

    char cmd[32];
    va_list args;

    // dump message
    va_start(args, msg);
	vprintf(msg, args);
    va_end(args);

    // make and exec system command
	sprintf(cmd, "cat /proc/%u/status", getpid());
	if (system(cmd) == -1) printf("[profile] unable to get process status.");
	fflush (stdout);
}
    

// ---------------------------------------------------------------------
// _parseLine
// ---------------------------------------------------------------------
static int _parseLine(char* line) {
    int i = strlen(line);
    while (*line < '0' || *line > '9') ++line;
    line[i-3] = '\0';
    i = atoi(line);
    return i;
}


// ---------------------------------------------------------------------
// get_vm_peak
// ---------------------------------------------------------------------
int get_vm_peak() {

    int result = -1;
    char line[128];

    FILE* file = fopen("/proc/self/status", "r");
    if (file == NULL) return -2;
    
    while (fgets(line, 128, file) != NULL)
        if (strncmp(line, "VmPeak:", 7) == 0) {
            result = _parseLine(line);
            break;
        }
 
    fclose(file);
 
    return result;
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
