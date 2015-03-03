/* ============================================================================
 *  pool.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 13, 2010
 *  Module:         dc

 *  Last changed:   $Date: 2010/10/15 09:41:01 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.3 $
*/

#include <stdio.h>
#include <stdlib.h>
#include "pool.h"


// ---------------------------------------------------------------------
// pool_init
// ---------------------------------------------------------------------
// create new allocation pool
//     size_t page_size: number of blocks in each page
//     size_t block_size: number of bytes per block (payload)
//     void** free_list_ref: address of void* object to contain the 
//                           list of free blocks
pool_t* pool_init(size_t page_size, size_t block_size, void** free_list_ref) {

    pool_t* p = (pool_t*)malloc(sizeof(pool_t));
    if (p == NULL) return NULL;

    p->first_page  = NULL;
    p->page_size   = page_size;
    p->block_size  = block_size;

    *free_list_ref = NULL;

    return p;
}


// ---------------------------------------------------------------------
// pool_cleanup
// ---------------------------------------------------------------------
// dispose of pool
//     pool_t* p: address of pool object previously created with pool_init
void pool_cleanup(pool_t* p) {
    void* ptr = p->first_page;
    while (ptr != NULL) {
        void* temp = ptr;
        ptr = *(void**)ptr;
        free(temp);
    }
    free(p);
}


// ---------------------------------------------------------------------
// pool_dump
// ---------------------------------------------------------------------
// print to stdout statistics about pool
//     pool_t* p: address of pool object previously created with pool_init
//     void* free_list: address of first free block
//     int dump_heap: if not zero, dumps also pages and blocks
void pool_dump(pool_t* p, void* free_list, int dump_heap) {

    size_t num_pages = 0, num_free = 0, i, j;
    void *page, *block;

    // count number of pages
    for (page = p->first_page; page != NULL; page = *(void**)page)
        num_pages++;

    // count number of free blocks
    for (block = free_list; block != NULL; block = *(void**)block)
        num_free++;

    printf("---- pool %p ----\n", p);
    printf(". number of blocks per page: %u\n", p->page_size);
    printf(". number of bytes per block: %u\n", p->block_size);
    printf(". number of pages allocated with malloc: %u\n", num_pages);
    printf(". number of blocks: %u\n", num_pages*p->page_size);
    printf(". number allocated blocks: %u\n", num_pages*p->page_size - num_free);
    printf(". number free blocks: %u\n", num_free);
    printf(". overall size of pool in bytes: %u (+%u libc malloc block headers)\n",
        sizeof(pool_t) + num_pages * (sizeof(void*) + p->page_size * p->block_size),
        num_pages);

    if (!dump_heap) return;

    // scan pages
    for ( page = p->first_page, j = 0; 
          page != NULL; page = *(void**)page, 
          ++j ) {

        void* limit;

        printf(". [page %u @ %p]:\n", j, page);

        // scan blocks
        limit = (char*)page + sizeof(void*) + p->page_size * p->block_size;
        for ( block = (char*)page + sizeof(void*);
              block != limit; 
              block = (char*)block + p->block_size ) {

            printf("    %p : [ ", block);
            for (i = 0; i < p->block_size; i++) {
                unsigned byte = ((unsigned char*)block)[i];
                printf("%s%s%X", i > 0 ? "-" : "", byte < 0x10 ? "0" : "" , byte);
            }
            printf(" ] %s\n", block == free_list ? "<-- first free block" : "");
        }
    }
}


// ---------------------------------------------------------------------
// _pool_add_page
// ---------------------------------------------------------------------
// allocate new page and carve new block from it (for internal use...)
void* _pool_add_page(pool_t* p, void** free_list_ref) {

    void *page;
    char *ptr, *limit;

    // allocate page
    page = malloc(sizeof(void*) + p->page_size * p->block_size);
    if (page == NULL) return NULL;

    // add page to page chain
    *(void**)page = p->first_page;
    p->first_page = page;

    // add all blocks in the page except the first one to the list of 
    // free blocks. Blocks are added to the front of the list in 
    // reverse order. The head of the list of free blocks is made to
    // point to the last block in the new page.
    ptr = (char*)page + sizeof(void*) + p->block_size;
    *(void**)ptr = *free_list_ref;
    limit = ptr + (p->page_size - 1) * p->block_size;
    for (ptr += p->block_size; ptr != limit; ptr += p->block_size)
        *(void**)ptr = (void*)(ptr - p->block_size);
    *free_list_ref = (void*)(limit - p->block_size);

    // allocate first block in page
    return (void*)((void**)page + 1);
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
