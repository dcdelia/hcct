#include <stdio.h>
#include <stdlib.h>

//#include "ceal.h"
//#include "modlist.h"

//item generator...
intptr_t rand_int() 
{
  return rand();
}

int isgt (void* x, void* y) 
{
    return (int)x>(int)y;
}