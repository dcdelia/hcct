#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/times.h>
#include <time.h>
#include <sys/param.h>

               
#include "rm.h"
#include "_dc.h"

typedef struct tagMyStruct
{
        unsigned long a, b, c, d, e;
} MyStruct;

MyStruct *p;

void *myShadowPtr;

int myConsID, myConsID2;

void myFinalHandler (void *inParam);

void myHandler (void *inParam)
{
  printf ("[HANDLER] entering...\n");
  
  p->a = p->b+p->c; 
  //printf ("MyConsID = %p\n", myConsID);
  
  schedule_final (get_cons_id (), myFinalHandler);
  
  printf ("[HANDLER] exiting...\n");
}

void myFinalHandler (void *inParam)
{
  printf ("[HANDLER] Final handler here...\n");
}

void myHandler2 (void *inParam)
{
  //printf ("[HANDLER] entering...\n");
  p->d = p->a; 
  //printf ("[HANDLER] exiting...\n");
  //schedule_final (myConsID, myFinalHandler);
}

void myfunc ()
{
    myShadowPtr=rm_get_shadow_rec (p);
}

void myfunc3 ()
{
    myShadowPtr=rm_get_shadow_rec_fast (p, long, MyStruct);
}

void myfunc2 ()
{
    myShadowPtr=rm_get_inactive_ptr (p);
}

int main ()
{   
    /*time structs...*/
    struct tms theTMS_begin, theTMS_end;
    time_t init_clock, end_clock;
    
    /*fetches initial times...*/
    times (&theTMS_begin);
    init_clock=time (NULL); 
    
    _dc_init (NULL);
    
    /*fetches final times...*/
    times (&theTMS_end);
    end_clock=time (NULL);
        
    /*reports times...*/
    printf ("\nCLOCKS_PER_SEC = %d\n", CLOCKS_PER_SEC);
    printf ("HZ = %d\n", HZ);
    printf ("Total elapsed time: %d seconds\n", end_clock-init_clock);
    printf ("User time: %f seconds\n", (double)((theTMS_end.tms_utime)-(theTMS_begin.tms_utime))/HZ);
    printf ("System time: %f seconds\n", (double)((theTMS_end.tms_stime)-(theTMS_begin.tms_stime))/HZ);
    printf ("User time (children): %f seconds\n", (double)((theTMS_end.tms_cutime)-(theTMS_begin.tms_cutime))/HZ);
    printf ("System time (children): %f seconds\n\n", (double)((theTMS_end.tms_cstime)-(theTMS_begin.tms_cstime))/HZ);
    
        p=rmalloc (sizeof (MyStruct));
        printf ("Allocated protected mem at %p\n", p);
        printf ("Address of p: %p\n", &p);
        printf ("Address of myShadowPtr: %p\n", &myShadowPtr);
        printf ("Address of g_shadow_wordsize: %p\n", &g_shadow_wordsize);
        printf ("Address of g_shadow_rec_size: %p\n", &g_shadow_rec_size);
        printf ("rm_heap_START_BRK: %p\n", rm_heap_START_BRK);
        printf ("rm_OFFSET: %p\n", rm_OFFSET);
        
        rm_make_dump_file("dump_test00.txt");
        
        p->b=1000;
        printf ("b set to 1000\n");
        p->c=500;
        printf ("c set to 500\n");
        
        printf ("Installing constraint a=b+c...\n");
        myConsID=newcons (myHandler, NULL);
        printf ("Constraint installed!\n");                
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        
        /*
        printf ("Installing constraint d=a...\n");
        myConsID2=newcons (myHandler2, NULL);
        printf ("Constraint installed!\n");                
        printf ("d = %u\n", p->d);
	
        printf ("Setting b=2000\n");
        p->b=2000;
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        printf ("d = %u\n", p->d);
	
        printf ("Setting c=1\n");
        p->c=1;
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        printf ("d = %u\n", p->d);
	
        printf ("Setting b=0\n");
        p->b=0;
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        printf ("d = %u\n", p->d);
        
        //delcons (myConsID); 
        //printf ("Constraint 1 deleted!\n");
        
        printf ("Setting b=100\n");
        p->b=100;
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        printf ("d = %u\n", p->d);
       
        printf ("Setting a=3000\n");  
        p->a=3000;
        printf ("a = %u\n", p->a);
        printf ("b = %u\n", p->b);
        printf ("c = %u\n", p->c);
        printf ("d = %u\n", p->d);   
         
        printf ("Setting a=3000...again!\n");  
        p->a=3000;
        */
        //printf ("Entering atomic sequence...\n");
        begin_at ();
        p->b=10;
        //printf ("a = %d\n", p->a);
        p->b=20;
       // printf ("a = %d\n", p->a);
        //p=NULL;
        p->b=3000;
        //printf ("a = %d\n", p->a);
        end_at ();
        //printf ("Exited atomic sequence!\n");
        printf ("b set to 3000\n");
        printf ("a = %d\n", p->a);
        printf ("b = %d\n", p->b);
        //delcons (myConsID2); 
        //printf ("Constraint 2 deleted!\n");
       
        rfree (p);
        
        printf ("--mem_read_count = %u\n", dc_stat.mem_read_count);
        printf ("--first_mem_read_count = %u\n", dc_stat.first_mem_read_count);
        printf ("--mem_write_count = %u\n", dc_stat.mem_write_count);
        printf ("--first_mem_write_count = %u\n", dc_stat.first_mem_write_count);
        printf ("--case_rw_count = %u\n", dc_stat.case_rw_count);
        printf ("--case_rw_first_count = %u\n", dc_stat.case_rw_first_count);
    
        
        return 0;
}
