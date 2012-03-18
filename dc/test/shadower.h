/* ============================================================================
 *  shadower.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/10/22 15:15:40 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.3 $
*/


#ifndef __shadower__
#define __shadower__

#include <stdlib.h>
#include <stddef.h>

// typdefs
typedef struct shadower_s shadower_t;
typedef struct shadow_node_s shadow_node_t;   

// handler types
typedef void (*con_t)(shadow_node_t* shadow_node);
typedef void (*des_t)(shadow_node_t* shadow_node);

// shadower object
struct shadower_s {
    shadow_node_t* generator;         // pointer to generator node of shadow list
    shadow_node_t* last_shadow_node;  // pointer to last node of shadow list
    size_t next_off;                  // offset of next field in shadowed nodes
    con_t con;                        // node client costructor
    des_t des;                        // node client destructor
    void* param;                      // client-defined parameter 
                                      // to be passed to con and des
    size_t rec_size;                  // size in bytes of shadow record
};

// shadow node record
struct shadow_node_s {
    void*  shadowed_node;             // pointer to shadowed node the shadow node
                                      // is associated to (NULL for generator node)
    void** next_shadowed_node;        // pointer to variable holding address of
                                      // successor of shadowed node (or address of first  
                                      // node in shadowed list for generator node)
    int cons_id;                      // constraint associated with shadow node
    shadower_t* shadower;             // shadower object the list belongs to
    shadow_node_t* next_shadow_node;  // pointer to next shadow node
    shadow_node_t* prev_shadow_node;  // pointer to previous shadow node
};


// ---------------------------------------------------------------------
// shadower_new
// ---------------------------------------------------------------------
/** create new list shadower
 * \param list_head address of pointer variable holding the head of shadowed list 
 * \param next_off offset in bytes of the next field in shadowed nodes
 * \param rec_size size in bytes of the shadow record to be associated with
 *        each shadowed node
 * \param con pointer to callback function (possibly NULL) to be called
 *        each time a newnode is added to the shadowed list
 * \param des pointer to callback function (possible NULL) to be called
 *        each time a newis removed from the shadowed list
 * \param param parameter to be passed as first argument to con and des
*/
shadower_t* shadower_new(
    void** list_head, size_t next_off,
    size_t rec_size, con_t con, des_t des, void* param);


// ---------------------------------------------------------------------
// shadower_delete
// ---------------------------------------------------------------------
/** delete list shadower
 *  \param shadower shadower object 
*/
void shadower_delete(shadower_t* shadower);


#if __DOXYGEN__
// ---------------------------------------------------------------------
// shadower_get_shadowed_node
// ---------------------------------------------------------------------
/** get pointer to shadowed node
 * \param shadow_node pointer to shadow node
 * \return pointer to shadowed node the shadow node is associated to
*/
void* shadow_node_get_shadowed_node(shadow_node_t* shadow_node);


// ---------------------------------------------------------------------
// shadower_get_next_shadow_node
// ---------------------------------------------------------------------
/** get pointer to next shadowed node
 *  \param shadow_node pointer to shadow node
 *  \return pointer to successor of shadowed node the shadow record is associated 
 *          to, or NULL if shadow_node is the last node in the shadow list
*/
shadow_node_t* shadow_node_get_next(shadow_node_t* shadow_node);


// ---------------------------------------------------------------------
// shadower_get_prev_shadow_node
// ---------------------------------------------------------------------
/** get pointer to previous shadowed node
 *  \param shadow_node pointer to shadow node
 *  \return pointer to predecessor of shadowed node the shadow record is associated 
 *          to, or NULL if shadow_node is the first node in the shadow list
*/
shadow_node_t* shadow_node_get_prev(shadow_node_t* shadow_node);


// ---------------------------------------------------------------------
// shadower_get_param
// ---------------------------------------------------------------------
/** get shadower the shadow node belongs to
 *  \param shadow_node pointer to shadow node
 *  \return pointer to shadower the shadow node belongs to
*/
shadower_t* shadow_node_get_shadower(shadow_node_t* shadow_node);


// ---------------------------------------------------------------------
// shadower_get_first_shadow_node
// ---------------------------------------------------------------------
/** get pointer to first node in shadowed list
 *  \param shadower pointer to shadower object
 *  \return pointer to first node in shadow list
*/
shadow_node_t* shadower_get_first_shadow_node(shadower_t* shadower);


// ---------------------------------------------------------------------
// shadower_get_last_shadow_node
// ---------------------------------------------------------------------
/** get pointer to last node in shadowed list
 *  \param shadower pointer to shadower object
 *  \return pointer to last node in shadow list
*/
shadow_node_t* shadower_get_last_shadow_node(shadower_t* shadower);


// ---------------------------------------------------------------------
// shadower_get_param
// ---------------------------------------------------------------------
/** get client parameter associated with shadower (param argument
 *  passed to shadower_new)
 *  \param shadower pointer to shadower object
 *  \return pointer to client parameter
*/
void* shadower_get_param(shadower_t* shadower);


// ---------------------------------------------------------------------
// shadower_get_rec_size
// ---------------------------------------------------------------------
/** get shadow record size associated with shadower (rec_size argument
 *  passed to shadower_new)
 *  \param shadower pointer to shadower object
 *  \return shadow record size
*/
size_t shadower_get_rec_size(shadower_t* shadower);


// ---------------------------------------------------------------------
// shadower_get_next_off
// ---------------------------------------------------------------------
/** get offset of next field in shadowed nodes (next_off argument
 *  passed to shadower_new)
 *  \param shadower pointer to shadower object
 *  \return next field offset in shadowed nodes
*/
size_t shadower_get_next_off(shadower_t* shadower);
#endif


#define shadow_node_get_shadowed_node(shadow_node) \
    ((shadow_node)->shadowed_node)

#define shadow_node_get_next(shadow_node) \
    ((shadow_node)->next_shadow_node)

#define shadow_node_get_prev(shadow_node) \
    ((shadow_node)->prev_shadow_node)

#define shadow_node_get_shadower(shadow_node) \
    ((shadow_node)->shadower)

#define shadower_get_first_shadow_node(shadower) \
    ((shadower)->generator->next_shadow_node)

#define shadower_get_last_shadow_node(shadower) \
    ((shadower)->last_shadow_node == (shadower)->generator ? \
        NULL : (shadower)->last_shadow_node)

#define shadower_get_param(shadower) \
    ((shadower)->param)

#define shadower_get_rec_size(shadower) \
    ((shadower)->rec_size)

#define shadower_get_next_off(shadower) \
    ((shadower)->next_off)
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
