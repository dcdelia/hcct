#include "ceal.h"
#include "modlist.h"
#include "mergesort.h"
#include "coin.h"

intptr_t rand_int ();

//imported from ceal's mergesort.c
afun mergesort_halve (coin_t* coin, cons_cell_t* cell, modref_t* dest1, modref_t* dest2);
