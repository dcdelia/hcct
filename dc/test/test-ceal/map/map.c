#include <stdio.h>
#include <stdlib.h>

//#include "ceal.h"
//#include "modlist.h"

//item generator...
intptr_t rand_int() 
{
  return rand();
}

//mapper func...
void* mapping (void* val) 
{
  intptr_t x = (intptr_t) val;
  x = (x / 3) + (x / 7) + (x / 9);  
  return (void*) x;
}
