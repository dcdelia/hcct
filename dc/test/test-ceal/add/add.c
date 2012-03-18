#include <stdio.h>
#include <stdlib.h>

//#include "ceal.h"
//#include "modlist.h"

//sum function...
intptr_t addint (void* dummy, intptr_t i1, intptr_t i2) 
{
  return i1 + i2;
}

//item generator...
intptr_t rand_int() 
{
  return rand();
}

