/* ============================================================================
 *  quicksorter.c
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 1, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/15 09:54:44 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/

#include <stdio.h>
#include <stdlib.h>

#include "dc.h"
#include "quicksorter.h"
#include "splitter.h"
#include "joiner.h"


// macros
#ifndef DEBUG
#define DEBUG 2
#endif


// intermediate lists
typedef struct {
    rnode_t* left_unsorted;         // unsorted left half
    rnode_t* right_unsorted;        // unsorted right half
    rnode_t* left_sorted;           // sorted left half
    rnode_t* right_sorted;          // sorted right half
} inter_t;


// object structure
struct quicksorter_s {
    int            cons_id;
    rnode_t**      in;              // input unsorted list
    rnode_t**      out;             // output sorted list
    inter_t*       inter;           // intermediate lists
    splitter_t*    splitter;        // splitter
    int            pivot;           // pivot
    joiner_t*      joiner;          // joiner
    quicksorter_t* left;            // left half sorter
    quicksorter_t* right;           // right half sorter
};


// ---------------------------------------------------------------------
//  dump_sorter
// ---------------------------------------------------------------------
static void dump_sorter(quicksorter_t* s) {
    printf("[quicksorter %p] dump\n", s);
    printf("   in: ");                 rlist_print(*s->in);
    if (s->inter != NULL) {
        printf("   left unsorted: ");  rlist_print(s->inter->left_unsorted);
        printf("   left sorted: ");    rlist_print(s->inter->left_sorted);
        printf("   right unsorted: "); rlist_print(s->inter->right_unsorted);
        printf("   right sorted: ");   rlist_print(s->inter->right_sorted);
    }
    printf("   out: ");                rlist_print(*s->out);
    if (s->inter != NULL) {
        printf("   splitter:        %p\n", s->splitter);
        printf("   left-subsorter:  %p\n", s->left);
        printf("   right-subsorter: %p\n", s->right);
        printf("   joiner:          %p\n", s->joiner);
    }
}


// ---------------------------------------------------------------------
//  quicksorter_cons
// ---------------------------------------------------------------------
static void quicksorter_cons(quicksorter_t* s) {

    #if DEBUG > 0
    printf("[quicksorter %p] executing cons for input list: ", s);
    rlist_inactive_print(*s->in);
    #endif

    // base step: input list is empty, or it contains only one item
    if (*s->in == NULL || (*s->in)->next == NULL) {

        // output list same as input list 
        *s->out = *s->in;

        // dispose of intermediate lists, if any
        if (s->inter != NULL) {
    
            #if DEBUG > 0
            printf("[quicksorter %p] deleting sub-sorters\n", s);
            #endif

            splitter_delete(s->splitter);
            quicksorter_delete(s->left);
            quicksorter_delete(s->right);
            joiner_delete(s->joiner);
            rfree(s->inter);
            s->inter = NULL;
        }
    }

    // list contains more than two items: maintain intermediate lists
    else if (s->inter == NULL) {

        #if DEBUG > 0
        printf("[quicksorter %p] creating sub-sorters\n", s);
        #endif

        s->pivot    = (*s->in)->val; // use first item as a pivot
        s->inter    = (inter_t*)rcalloc(sizeof(inter_t), 1);
        s->splitter = splitter_new(s->in, &s->inter->left_unsorted, &s->inter->right_unsorted, &s->pivot);
        s->left     = quicksorter_new(&s->inter->left_unsorted,  &s->inter->left_sorted);
        s->right    = quicksorter_new(&s->inter->right_unsorted, &s->inter->right_sorted);
        s->joiner   = joiner_new(&s->inter->left_sorted, &s->inter->right_sorted, s->out);

        if (s->inter == NULL || s->splitter == NULL || s->joiner == NULL || 
               s->left == NULL || s->right == NULL) {
            printf("[quicksorter] out of memory\n");
            exit(1);
        }
    }

    #if DEBUG > 1
    schedule_final(get_cons_id(), (cons_t)dump_sorter);
    #endif

    #if DEBUG > 2
    printf("[quicksorter %p] -- end cons\n", s);
    #endif
}


// ---------------------------------------------------------------------
// quicksorter_new
// ---------------------------------------------------------------------
quicksorter_t* quicksorter_new(rnode_t** in, rnode_t** out) {

    // create sorter object
    quicksorter_t* s = (quicksorter_t*)malloc(sizeof(quicksorter_t));
    if (s == NULL) return NULL;

    #if DEBUG > 0
    printf("[quicksorter %p] created for input list head: ", s);
    if (*in == NULL) printf("NULL\n"); 
    else printf("%d\n", (*in)->val);
    #endif

    // setup in and out lists
    s->in  = in;
    s->out = out;

    // no intermediate lists
    s->inter = NULL;

    // create constraint
    s->cons_id = newcons((cons_t)quicksorter_cons, s);

    return s;
}


// ---------------------------------------------------------------------
// quicksorter_delete
// ---------------------------------------------------------------------
void quicksorter_delete(quicksorter_t* s) {

    // dispose of intermediate lists, if any
    if (s->inter) {
        begin_at();
        splitter_delete(s->splitter);
        quicksorter_delete(s->left);
        quicksorter_delete(s->right);
        joiner_delete(s->joiner);
        rfree(s->inter);
        end_at();
    }

    #if DEBUG > 0
    printf("[quicksorter %p] deleted\n", s);
    #endif

    free(s);
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
