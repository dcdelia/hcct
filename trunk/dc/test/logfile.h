/* ============================================================================
 *  logfile.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/26 07:19:36 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/


#ifndef __logfile__
#define __logfile__

#define LOGFILE_MAX_BUF_LEN 128

typedef struct logfile_s logfile_t;

// ---------------------------------------------------------------------
//  logfile_new
// ---------------------------------------------------------------------
// create log file or append to existing one
logfile_t* logfile_open(const char* file_name, int append);


// ---------------------------------------------------------------------
//  logfile_add_to_header
// ---------------------------------------------------------------------
// add col name to header
int logfile_add_to_header(logfile_t* f, const char* format, ...);


// ---------------------------------------------------------------------
//  logfile_add_to_row
// ---------------------------------------------------------------------
// add cell value to row
int logfile_add_to_row(logfile_t* f, const char* format, ...);


// ---------------------------------------------------------------------
//  logfile_new_row
// ---------------------------------------------------------------------
// flush current row and start new one
void logfile_new_row(logfile_t* f);


// ---------------------------------------------------------------------
//  logfile_delete
// ---------------------------------------------------------------------
// flush current row and close file
void logfile_close(logfile_t* f);

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
