#include <stdlib.h>

#include "common.h"
#include "metrics.h"

UINT32 countNodes(hcct_node_t* root) {
    if (root == NULL) return 0;
    UINT32 ret = 1;
    hcct_node_t* child;
    for (child = root->first_child; child != NULL; child = child->next_sibling)
        ret += countNodes(child);
    return ret;
}

// ---------------------------------------------------------------------
//  routines adapted from metrics.c (PLDI version)
// ---------------------------------------------------------------------

UINT32 hotNodes(hcct_node_t* root, UINT32 threshold) {
    UINT32 h = 0;
    hcct_node_t* child;
    for (child = root->first_child; child != NULL; child = child->next_sibling)
        h += hotNodes(child, threshold);
    if (root->counter > threshold) {
        // printf per stampa a video??
        return h + 1;
    } else return h;
}

UINT64 sumCounters(hcct_node_t* root) {
    UINT64 theSum = (UINT64) root->counter;
    hcct_node_t* child;
    for (child = root->first_child; child != NULL; child = child->next_sibling)
        theSum += sumCounters(child);
    return theSum;
}

UINT32 hottestCounter(hcct_node_t* root) {
    UINT32 theHottest = root->counter;
    hcct_node_t* child;
    for (child = root->first_child; child != NULL; child = child->next_sibling) {
        UINT32 childHottest = hottestCounter(child);
        if (theHottest < childHottest) theHottest = childHottest;
    }
    return theHottest;
}

UINT32 largerThanHottest(hcct_node_t* root, UINT32 TAU, UINT32 hottest) {
    UINT32 num = 0;
    hcct_node_t* child;
    if (root->counter * TAU >= hottest) num++;
    for (child = root->first_child; child != NULL; child = child->next_sibling)
        num += largerThanHottest(child, TAU, hottest);
    return num;
}

// ---------------------------------------------------------------------
//  end of routines adapted from metrics.c (PLDI version)
// ---------------------------------------------------------------------
