/* ============================================================================
 *  vartracker.cpp
 * ============================================================================

 *  Author:         (c) 2010 Camil Demetrescu
 *  License:        See the end of this file for license information
 *  Created:        October 20, 2010
 *  Module:         dc/test

 *  Last changed:   $Date: 2010/11/16 12:48:49 $
 *  Changed by:     $Author: demetres $
 *  Revision:       $Revision: 1.3 $
*/

#include "vartracker.h"

int main() {
    
    volatile long* var = (long*)rmalloc(sizeof(long));

    vartracker<long> mytracker(var);

    *var = 10;
    *var = 20;

    mytracker.disable();

    (*var)++;
    
    return 0;
}
