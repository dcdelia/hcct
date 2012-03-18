/* ============================================================================
 *  logfile.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 25, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/26 07:19:35 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "logfile.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct entry_s {
    char val[LOGFILE_MAX_BUF_LEN];
    struct entry_s* next;
} entry_t;

struct logfile_s  {
    FILE*    file;
    entry_t* header;
    entry_t* row;
    int      print_row;
};


// ---------------------------------------------------------------------
//  flush_row
// ---------------------------------------------------------------------
static void flush_row(logfile_t* f, entry_t** r) {
    if (*r == NULL) return;
    else {
        flush_row(f, &(*r)->next);
        fprintf(f->file, "%s", (*r)->val);
        free(*r);
        *r = NULL;
    }
}


// ---------------------------------------------------------------------
//  add_to_list
// ---------------------------------------------------------------------
static int add_to_list(entry_t** head, const char* format, va_list args) {
    entry_t* entry = (entry_t*)malloc(sizeof(entry_t));
    if (entry == NULL) return -1;
    entry->next = *head;
    vsprintf(entry->val, format, args);
    *head = entry;
    return 0;
}


// ---------------------------------------------------------------------
//  logfile_new
// ---------------------------------------------------------------------
// create log file or append to existing one
logfile_t* logfile_open(const char* file_name, int append) {
    
    logfile_t* f = (logfile_t*)malloc(sizeof(logfile_t));
    if (f==NULL) return NULL;

    // if file does not exist, then always print header, else
    // print it only if append == 0
    f->file = fopen(file_name, "r");
    if (f->file == NULL) f->print_row = 1;
    else {
        fclose(f->file);
        f->print_row = !append;
    }

    f->file = fopen(file_name, append ? "a" : "w");
    if (!f->file) {
        free(f);
        return NULL;
    }
    
    f->header = f->row = NULL;

    return f;
}


// ---------------------------------------------------------------------
//  logfile_add_to_header
// ---------------------------------------------------------------------
// add col name to header
int logfile_add_to_header(logfile_t* f, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = add_to_list(&f->header, format, args);
    va_end(args);
    return res;
}


// ---------------------------------------------------------------------
//  logfile_add_to_row
// ---------------------------------------------------------------------
// add cell value to row
int logfile_add_to_row(logfile_t* f, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = add_to_list(&f->row, format, args);
    va_end(args);
    return res;
}


// ---------------------------------------------------------------------
//  logfile_new_row
// ---------------------------------------------------------------------
// flush current row and start new one
void logfile_new_row(logfile_t* f) {
    if (f->print_row && f->header) {
        flush_row(f, &f->header);
        fprintf(f->file, "\n");        
    }
    if (f->row) {
        flush_row(f, &f->row);
        fprintf(f->file, "\n");        
    }
}


// ---------------------------------------------------------------------
//  logfile_delete
// ---------------------------------------------------------------------
// flush current row and close file
void logfile_close(logfile_t* f) {
    logfile_new_row(f);
    fclose(f->file);
    free(f);
}


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
