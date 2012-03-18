/* ============================================================================
 *  dcpp.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 22:47:23 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.4 $
*/

#ifndef __DCPP__
#define __DCPP__

#include "dc.h"

#define DEBUG 0

#if DEBUG
#include <iostream>
#endif


// ---------------------------------------------------------------------
// robject
// ---------------------------------------------------------------------
struct robject {
    void* operator new(size_t size) {
        #if DEBUG
        std::cout << "[robject::new] allocating " << size << " bytes\n";
        #endif
        return rmalloc(size);
    }

    void operator delete(void* ptr) {
        rfree(ptr);
        #if DEBUG
        std::cout << "[robject::delete] robject deleted\n";
        #endif
    }
};


// ---------------------------------------------------------------------
// rcons
// ---------------------------------------------------------------------

static void con_h(void*), fin_h(void*);

class rcons {
    int id;
  public:
  
    void* vptr;

    rcons() { 
        id = -1;
    }

    ~rcons() { 
        #if DEBUG
        std::cout << "[rcons::~rcons] deleting cons\n"; 
        #endif
        disable(); 
    }

    virtual void cons() = 0;

    virtual void final() { }

    void enable() {
        #if DEBUG
        std::cout << "[rcons::enable] enabling cons\n";
        #endif
        if (id != -1) return;
        vptr = *(void**)this;
        id = newcons(con_h, this);
    }

    void disable() {
        #if DEBUG
        std::cout << "[rcons::disable] disabling cons\n";
        #endif
        if (id == -1) return;
        delcons(id);
        id = -1;
    }
    
    void arm_final() {
        if (id != -1) schedule_final(id, fin_h);
    }

    void unarm_final() {
        if (id != -1) schedule_final(id, NULL);
    }
};

void con_h (void* p) { 
    
    #if DEBUG
    std::cout << "[cons_h] calling cons - orig vptr=" 
              << ((rcons*)p)->vptr  
              << " - curr vptr=" 
              << *(void**)p << std::endl;  
    #endif

    if (*(void**)p == ((rcons*)p)->vptr) 
       ((rcons*)p)->cons();  
}

void fin_h (void* p) { 
    
    #if DEBUG
    std::cout << "[cons_h] calling cons - orig vptr=" 
              << ((rcons*)p)->vptr  
              << " - curr vptr=" 
              << *(void**)p << std::endl;  
    #endif

    if (*(void**)p == ((rcons*)p)->vptr) 
       ((rcons*)p)->final();  
}

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
*/
