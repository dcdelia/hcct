/* ============================================================================
 *  dc.h
 * ============================================================================

 *  Author:         (c) 2008 Camil Demetrescu, Andrea Ribichini
 *  License:        See the end of this file for license information
 *  Created:        January 24, 2008
 *  Module:         ??

 *  Last changed:   $Date: 2010/11/15 13:23:37 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.12 $
*/


#ifndef __dc__
#define __dc__

#include "rm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DC_STAT
#define DC_STAT 1
#endif

typedef struct {
    unsigned mem_read_count;
    unsigned first_mem_read_count;
    unsigned mem_write_count;
    unsigned first_mem_write_count;
    
    unsigned case_rw_count;
    unsigned case_rw_first_count;
} dc_stat_t;

extern dc_stat_t dc_stat;

// typedefs
typedef void (*cons_t) (void* inParam);
typedef char (*comparator_t) (unsigned long inA, unsigned long inB);


// ---------------------------------------------------------------------
//  rmalloc
// ---------------------------------------------------------------------
/** Allocate reactive memory block. Allocates a block of size bytes of reactive memory, 
    returning a pointer to the beginning of the block. The content of the newly 
    allocated block of memory is not initialized, remaining with indeterminate values.
    The semantics is identical to the C malloc function, except that rmalloc 
    allocates a reactive memory block.
    \param size size of the memory block, in bytes
    \return On success, a pointer to the reactive memory block allocated by the 
            function. The type of this pointer is always void*, which can be cast 
            to the desired type of data pointer in order to be dereferenceable.
            If the function failed to allocate the requested block of memory, a 
            null pointer is returned.
*/
void* rmalloc(size_t size);


// ---------------------------------------------------------------------
//  rcalloc
// ---------------------------------------------------------------------
/** Allocate space for array in reactive memory. Allocates a block of reactive memory 
    for an array of num elements, each of them size bytes long, and initializes all its 
    bits to zero. The effective result is the allocation of an zero-initialized memory 
    block of (num*size) bytes. Initialization does not generate memory write events.
    The semantics is identical to the C calloc function, except that rcalloc 
    allocates a reactive memory block.
    \param num number of array elements to be allocated
    \param size size of array elements, in bytes
    \return a pointer to the reactive memory block allocated by the function. The type 
    of this pointer is always void*, which can be cast to the desired type of data pointer 
    in order to be dereferenceable. If the function failed to allocate the requested block 
    of memory, a NULL pointer is returned.
*/
void* rcalloc(size_t num, size_t size);


// ---------------------------------------------------------------------
//  rrealloc
// ---------------------------------------------------------------------
/** Reallocate reactive memory block. The size of the memory block pointed to by the ptr 
    parameter is changed to the size bytes, expanding or reducing the amount of memory 
    available in the block. The function may move the memory block to a new location, in 
    which case the new location is returned. The content of the memory block is preserved 
    up to the lesser of the new and old sizes, even if the block is moved. If the new size 
    is larger, the value of the newly allocated portion is indeterminate. In case that ptr 
    is NULL, the function behaves exactly as malloc, assigning a new block of size bytes 
    and returning a pointer to the beginning of it. In case that the size is 0, the reactive
    memory previously allocated in ptr is deallocated as if a call to free was made, and a 
    NULL pointer is returned.
    The semantics is identical to the C realloc function, except that rrealloc 
    reallocates a reactive memory block.
    \param addr memory address
    \return non-zero if addr is reactive, zero otherwise
*/
void* rrealloc(void* ptr, size_t size);


// ---------------------------------------------------------------------
//  rfree
// ---------------------------------------------------------------------
/** Deallocate space in reactive memory. A block of reactive memory previously allocated 
    using a call to rm_malloc, rm_calloc, or rm_realloc is deallocated, making it available 
    again for further allocations. Notice that this function leaves the value of ptr unchanged, 
    hence it still points to the same (now invalid) location, and not to the null pointer.
    The semantics is identical to the C free function, except that rfree 
    frees a reactive memory block.
    \param ptr pointer to a memory block previously allocated with rmalloc, rcalloc or rrealloc to 
           be deallocated. If a null pointer is passed as argument, no action occurs.
*/
void rfree(void* ptr);


// ---------------------------------------------------------------------
//  newcons
// ---------------------------------------------------------------------
int newcons(cons_t inHandler, void *inParam);

// ---------------------------------------------------------------------
//  schedule_final
// ---------------------------------------------------------------------
int schedule_final(int cons_id, cons_t inFinalHandler);

// ---------------------------------------------------------------------
//  get_cons_id
// ---------------------------------------------------------------------
int get_cons_id();

// ---------------------------------------------------------------------
//  delcons
// ---------------------------------------------------------------------
void delcons(int cons_id);


// ---------------------------------------------------------------------
//  begin_at
// ---------------------------------------------------------------------
int begin_at();


// ---------------------------------------------------------------------
//  end_at
// ---------------------------------------------------------------------
int	end_at();


// ---------------------------------------------------------------------
//  dc_num_cons
// ---------------------------------------------------------------------
unsigned long dc_num_cons();


// ---------------------------------------------------------------------
//  dc_num_exec_cons
// ---------------------------------------------------------------------
unsigned long dc_num_exec_cons();


// ---------------------------------------------------------------------
//  dc_dump_arcs
// ---------------------------------------------------------------------
// dumps dependencies for address addr
void dc_dump_arcs(void* addr);

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
