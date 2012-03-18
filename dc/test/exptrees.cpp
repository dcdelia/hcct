/* ============================================================================
 *  exptrees.cpp
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        November 16, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 12:48:49 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.1 $
*/

#include "dcpp.h"
#include "vartracker.h"
#include <iostream>

using namespace std;

enum op_t { SUM, PROD };

template<typename T> struct node : robject, rcons {
    T value;
    op_t op;
    node *left, *right;
    node(T v): value(v), left(NULL), right(NULL) { enable(); }
    node(op_t o): op(o), left(NULL), right(NULL) { enable(); }
    void cons() {
        if (left == NULL || right == NULL) return;
        switch (op) {
            case SUM:  value = left->value + right->value; break;
            case PROD: value = left->value * right->value; break;
        }
    }
};

int main() {
    node<int>* root = new node<int>(SUM);
    vartracker<int> v(&root->value);
    root->left  = new node<int>(10);
    root->right = new node<int>(PROD);
    root->right->left  = new node<int>(2);
    root->right->right = new node<int>(6);
//    cout << "root value: " << root->value << endl;
    root->right->op = SUM;
//    cout << "root value: " << root->value << endl;
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
