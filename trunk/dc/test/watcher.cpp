/* ============================================================================
 *  watcher.h
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 15, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/18 22:23:53 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.2 $
*/


#include "dcpp.h"
#include <stdio.h>
#include <iostream>
#include <map>

using namespace std;

struct node : public robject {
    int elem;
    node *next, *prev;
};

void add_first(node* head, node** tail, int elem) {
    node* v = new node();
    v->elem = elem;
    v->next = *tail;
    v->prev = head;
    if (*tail != NULL) (*tail)->prev = v;
    *tail = v;
}

#if 0
template<typename T> class snode : public rcons {
    typedef      map<node**,snode<T>*> smap;
    node        *head, **tail;
    smap*        table;
    snode* next;
    int          refc;

  public:
    snode(node* h, node** t, smap* m) :
            head(h), tail(t), table(m), next(NULL), refc(0) {
        (*table)[tail] = this;
        enable();
        printf("[watcher %p] constructor [added pair (%p,%p) to table]\n", this, tail, this);
    }

   ~snode() { 
        printf("[watcher %p] destructor\n", this);
        table->erase(tail); 
        if (next != NULL && --next->refc == 0) delete next;
    }

    void cons() {
        snode<T>* cur_next;

        printf("[watcher %p] ", this);
        if (head == NULL) printf("cons execution for generator node\n");
        else printf("cons execution for input node %d\n", head->elem);

        if (*tail != NULL) {

            typename smap::iterator it = table->find(&(*tail)->next);

            printf("[watcher %p] looking up key %p\n", this, &(*tail)->next);
            printf("[watcher %p] key %s found\n", this, it == table->end() ? "not":"");
            if (it == table->end())
                 cur_next = new snode<T>(*tail, &(*tail)->next, table);
            else {
                cur_next = it->second;
                printf("[watcher %p] found next node val %d\n", this, cur_next->head->elem);
            }
        }
        else cur_next = NULL;

        if (next != cur_next) {
            if (next != NULL && --next->refc == 0)
                next->arm_final();
            if (cur_next != NULL && cur_next->refc++ == 0)
                cur_next->unarm_final();
            next = cur_next;
        }

        T::analyze(head, next == NULL ? NULL : next->head);
    }

    void final() {
        printf("[watcher %p] final\n", this);
        delete this;
    }
};
#else
template<class T, class N> class snode : public rcons {
    map<N**, snode<T,N>*> *m;
    N     *head, **tail;
    snode *next;
    int    refc;

  public:
    snode(N* h, N** t, map<N**, snode<T,N>*>* m) :
        m(m), head(h), tail(t), next(NULL), refc(0) { 
        (*m)[tail] = this; 
        enable(); 
    }

   ~snode() { 
        m->erase(tail); 
        if (next != NULL && --next->refc == 0) delete next;
    }

    void cons() {
        snode<T,N>* cur_next;

        if (*tail != NULL) {
            typename map<N**, snode<T,N>*>::iterator it =
                m->find( &(*tail)->next );
            if (it != m->end()) 
                 cur_next = it->second;
            else cur_next = new snode<T,N>(*tail, 
                                         &(*tail)->next, m);
        } else cur_next = NULL;

        if (next != cur_next) {
            if (next != NULL && --next->refc == 0)
                next->arm_final();
            if (cur_next != NULL && cur_next->refc++ == 0)
                cur_next->unarm_final();
            next = cur_next;
        }

        if (head != NULL) T::watch(head);
    }

    void final() { delete this; }
};
#endif

template<class T, class N> class watcher {
    snode<T,N>* gen;
    map<N**, snode<T,N>*> m;
  public:
    watcher(N** h) { gen = new snode<T,N>(NULL, h, &m); }
   ~watcher()      { delete gen; }
};

#if 1
struct myrepairer {
    static void watch(node* x) {
        if (x->next != NULL && x != x->next->prev) {
            x->next->prev = x;
            cout << "*** detected inconsistent prev pointer\n"
                 << "    error repaired.\n";
        }
    }
};
#else
struct myrepairer {
    static void watch(node* x, node* y) {
        if (y != NULL && y->prev != x) 
            y->prev = x;
    }
};
#endif

#if 0
struct mybubblesorter {
    static void watch(node* x, node* y) {
        if (x != NULL && y != NULL && x->elem > y->elem) {
            int temp = x->elem;
            x->elem = y->elem;
            y->elem = temp;
        }
    }
};
#else
struct mybubblesorter {
    static void watch(node* x, node* y) {
        printf("watching %p %p\n", x, y);
        if (x != NULL && y != NULL) {
            printf("   %d %d\n", x->elem, y->elem);
            if (x->elem > y->elem) {
                int temp = x->elem;
                x->elem = y->elem;
                y->elem = temp;
            }
        }
    }
};
#endif

void print_list(node* p) {    
    for (; p != NULL; p = p->next) printf("%d ", p->elem);
    printf("\n");
}

int main() {

    node** list = (node**)rmalloc(sizeof(node*));
    *list = NULL;

    watcher<myrepairer,node> r(list);

    begin_at();
    add_first(NULL, list, 9);
    add_first(NULL, list, 10);
    add_first(NULL, list, 20);
    add_first(NULL, list, 5);
    end_at();

    (*list)->next->next->prev = NULL;

    node* p;
    for (p = *list; p != NULL; p = p->next)
        if (p->next != NULL && p->next->prev != p) break;
    printf("result: %s\n", p == NULL ? "ok" : "error");

    #if 0
    watcher<mybubblesorter> b(list);

    print_list(*list);
    
    (*list)->elem = 120;
    
    print_list(*list);
    #endif

    return 0;
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
*/
