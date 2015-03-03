#include "common.h"

/* The following code has been adapted from Martin Broadhurst's mbcomb library:
 * http://www.martinbroadhurst.com/combinatorial-algorithms.html 
 */

static inline void permutation_swap(UINT32 *ar, UINT32 first, UINT32 second);
static inline void permutation_reverse(UINT32 *ar, UINT32 len);

UINT32 next_combination(UINT32 *ar, UINT32 n, UINT32 k) {
    UINT32 finished = 0;
    UINT32 changed = 0;
    UINT32 i;

    if (k > 0) {
        for (i = k - 1; !finished && !changed; i--) {
            if (ar[i] < (n - 1) - (k - 1) + i) {
                /* Increment this element */
                ar[i]++;
                if (i < k - 1) {
                    /* Turn the elements after it into a linear sequence */
                    UINT32 j;
                    for (j = i + 1; j < k; j++) {
                        ar[j] = ar[j - 1] + 1;
                    }
                }
                changed = 1;
            }
            finished = i == 0;
        }
        if (!changed) {
            /* Reset to first combination */
            for (i = 0; i < k; i++) {
                ar[i] = i;
            }
        }
    }
    return changed;
}

// works only for len > 1
UINT32 next_permutation(UINT32 *ar, UINT32 len) {
    UINT32 i1, i2;
    UINT32 result = 0;
    
    /* Find the rightmost element that is the first in a pair in ascending order */
    for (i1 = len - 2, i2 = len - 1; ar[i2] <= ar[i1] && i1 != 0; i1--, i2--);
    if (ar[i2] <= ar[i1]) {
        /* If not found, array is highest permutation */
        permutation_reverse(ar, len);
    }
    else {
        /* Find the rightmost element to the right of i1 that is greater than ar[i1] */
        for (i2 = len - 1; i2 > i1 && ar[i2] <= ar[i1]; i2--);
        /* Swap it with the first one */
        permutation_swap(ar, i1, i2);
        /* Reverse the remainder */
        permutation_reverse(ar + i1 + 1, len - i1 - 1);
        result = 1;
    }
    return result;
}

// works only for k > 1
UINT32 next_k_permutation(UINT32 *ar, UINT32 n, UINT32 k) {
    UINT32 result = next_permutation(ar, k);
    if (result == 0) {
        result = next_combination(ar, n, k);
    }
    return result;
}

static inline void permutation_swap(UINT32 *ar, UINT32 first, UINT32 second) {
    UINT32 temp = ar[first];
    ar[first] = ar[second];
    ar[second] = temp;
}

static inline void permutation_reverse(UINT32 *ar, UINT32 len) {
    UINT32 i, j;
    for (i = 0, j = len - 1; i < j; i++, j--) {
        permutation_swap(ar, i, j);
    }
}