/* ============================================================================
 *  pool.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc

 *  Last changed:   $Date: 2010/10/15 09:41:25 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/


#ifndef __pool__
#define __pool__

#ifdef __cplusplus
extern "C" {
#endif


// pool object structure
typedef struct {
    void**   first_page;    // pointer to first page
    size_t   page_size;     // page size as number of blocks
    size_t   block_size;    // block size in bytes
} pool_t;


// ---------------------------------------------------------------------
// pool_init
// ---------------------------------------------------------------------
// create new allocation pool
//     size_t page_size: number (>= 2) of blocks in each page
//     size_t block_size: number (>= 4) of bytes per block (payload)
//     void** free_list_ref: address of void* object to contain the 
//                           list of free blocks
pool_t* pool_init(size_t page_size, size_t block_size, void** free_list_ref);


// ---------------------------------------------------------------------
// pool_cleanup
// ---------------------------------------------------------------------
// dispose of pool
//     pool_t* p: address of pool object previously created with pool_init
void pool_cleanup(pool_t* p);


// ---------------------------------------------------------------------
// pool_dump
// ---------------------------------------------------------------------
// print to stdout statistics about pool
//     pool_t* p: address of pool object previously created with pool_init
//     void* free_list: address of first free block
//     int dump_heap: if not zero, dumps also pages and blocks
void pool_dump(pool_t* p, void* free_list, int dump_heap);


// ---------------------------------------------------------------------
// _pool_add_page
// ---------------------------------------------------------------------
// for internal use...
void* _pool_add_page(pool_t* p, void** free_list);


// ---------------------------------------------------------------------
// pool_alloc (macro)
// ---------------------------------------------------------------------
// allocate new block
//     pool_t* pool: address of pool object created with pool_init
//     void* free_list: lvalue containing the address of first block in 
//                      a list of free blocks
//     void* out: lvalue to be assigned with the address of the newly 
//                allocated block
#define pool_alloc(pool, free_list, out, block_type) \
    (free_list ? ((out) = (block_type*)(free_list), (free_list) = *(void**)(free_list)) : \
                 ((out) = (block_type*)_pool_add_page((pool), &free_list)))


// ---------------------------------------------------------------------
// pool_free (macro)
// ---------------------------------------------------------------------
// free block
//     void* block_ptr: address of block to be freed
//     void* free_list: lvalue containing the address of the first block
//                      in a list of free blocks 
#define pool_free(block_ptr, free_list) \
    (*(void**)(block_ptr) = (free_list), (free_list) = (block_ptr))


#ifdef __cplusplus
}
#endif

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
