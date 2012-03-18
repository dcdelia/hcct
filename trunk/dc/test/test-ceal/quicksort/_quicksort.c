#include <stdio.h>
#include <stdlib.h>

//#include "ceal.h"
//#include "modlist.h"

//item generator...
intptr_t rand_int() 
{
  return rand();
}

//comparators...
int isle (void* v1, void* v2) 
{
  return ((intptr_t) v1) <= ((intptr_t) v2);
}


int isgt (void* v1, void* v2) 
{
  return ((intptr_t) v1) > ((intptr_t) v2);
}
