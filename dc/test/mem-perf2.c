/****************************************************************
**
** mem_perf2
**
****************************************************************/

/*INCLUDES...*/

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

//#include "LWP.h"
//#include "LCP.h" 
#include "dc.h"

/*DEFINES...*/

#define N 1000000
#define NUM_PAGES 10000

int main ()
{
	volatile int *prot_mem;
	volatile int *unprot_mem;
	
	int i;
	double randomNumber;
	int value, index;
	
	double prot_write_times;
	double prot_read_times;
	double unprot_write_times;
	double unprot_read_times;
	
	struct timeval tvBeginProtWrite, tvEndProtWrite;
	struct timeval tvBeginProtRead, tvEndProtRead;
	struct timeval tvBeginUnprotWrite, tvEndUnprotWrite;
	struct timeval tvBeginUnprotRead, tvEndUnprotRead;
	
	printf ("N is %d\n", N);
	printf ("NUM_PAGES is %d\n", NUM_PAGES);
	
	/*generates random seed...*/
	srand((unsigned)time (NULL));
	//srand (140173);
	
	/*allocates protected mem...*/
	prot_mem=(int *)rmalloc (4096*NUM_PAGES); 
	if (prot_mem==NULL)
		printf ("ERROR: Could not allocate protected mem!\n");
	
	/*allocates unprotected mem...*/
	unprot_mem=(int *)malloc (NUM_PAGES*4096);
	if (unprot_mem==NULL)
		printf ("ERROR: Could not allocate unprotected mem!\n");
		
	printf ("Protected write loop ");
	fflush (stdout);
	/*generates random number in (0,1)...*/
	randomNumber=rand ()/(RAND_MAX+1.0);		
	value=randomNumber*0xffffffff;
	/*fetches initial time...*/
	gettimeofday (&tvBeginProtWrite, NULL);
	/*performs N writes to prot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*4096/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		prot_mem[index]=value;
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndProtWrite, NULL);
	prot_write_times=tvEndProtWrite.tv_sec+(tvEndProtWrite.tv_usec*0.000001)
			-tvBeginProtWrite.tv_sec-(tvBeginProtWrite.tv_usec*0.000001);	
				
	printf ("Unprotected write loop ");
	fflush (stdout);
	/*generates random number in (0,1)...*/
	randomNumber=rand ()/(RAND_MAX+1.0);		
	value=randomNumber*0xffffffff;
	/*fetches initial time...*/
	gettimeofday (&tvBeginUnprotWrite, NULL);
	/*performs N writes to unprot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*4096/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		unprot_mem[index]=value;
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndUnprotWrite, NULL);
	unprot_write_times=tvEndUnprotWrite.tv_sec+(tvEndUnprotWrite.tv_usec*0.000001)
			-tvBeginUnprotWrite.tv_sec-(tvBeginUnprotWrite.tv_usec*0.000001);		
	
	printf ("Protected read loop ");
	fflush (stdout);
	/*fetches initial time...*/
	gettimeofday (&tvBeginProtRead, NULL);
	/*performs N reads from prot mem...*/
	for (i=0; i<N; i++)
	{	
		
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*4096/4;
		//printf ("Index=%d\n", index);
	
		/*performs access...*/
		value=prot_mem[index];	
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndProtRead, NULL);
	prot_read_times=tvEndProtRead.tv_sec+(tvEndProtRead.tv_usec*0.000001)
			-tvBeginProtRead.tv_sec-(tvBeginProtRead.tv_usec*0.000001);
				
	printf ("Unprotected read loop ");
	fflush (stdout);
	/*fetches initial time...*/
	gettimeofday (&tvBeginUnprotRead, NULL);
	/*performs N reads from unprot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*4096/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		value=unprot_mem[index];
			
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndUnprotRead, NULL);
	unprot_read_times=tvEndUnprotRead.tv_sec+(tvEndUnprotRead.tv_usec*0.000001)
			-tvBeginUnprotRead.tv_sec-(tvBeginUnprotRead.tv_usec*0.000001);
	
	
	/*prints results...*/
	
	printf ("Seconds taken for %d protected writes: %f\n", N, prot_write_times);
	
	printf ("Seconds taken for %d unprotected writes: %f\n", N, unprot_write_times);
	
	printf ("Seconds taken for %d protected reads: %f\n", N, prot_read_times);
	
	printf ("Seconds taken for %d unprotected reads: %f\n", N, unprot_read_times);
	
	printf ("Ratio between prot and unprot writes: %f\n", prot_write_times/unprot_write_times);
	
	printf ("Ratio between prot and unprot reads: %f\n", prot_read_times/unprot_read_times);
	
#if 0
	//resets random generator...
	srand (140173);
	//LCP_CleanUp ();
	//LCP_Init (100000000);
	//printf ("\nLCP reset...\n");
	printf ("\nKeeping LCP data...\n");
	
	//runs tests all over again...
	printf ("Second run...\n");
		
	printf ("Protected write loop ");
	fflush (stdout);
	/*generates random number in (0,1)...*/
	randomNumber=rand ()/(RAND_MAX+1.0);		
	value=randomNumber*0xffffffff;
	/*fetches initial time...*/
	gettimeofday (&tvBeginProtWrite, NULL);
	/*performs N writes to prot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*LWP_GetPageSize ()/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		prot_mem[index]=value;
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndProtWrite, NULL);
	prot_write_times=tvEndProtWrite.tv_sec+(tvEndProtWrite.tv_usec*0.000001)
			-tvBeginProtWrite.tv_sec-(tvBeginProtWrite.tv_usec*0.000001);	
				
	printf ("Unprotected write loop ");
	fflush (stdout);
	/*generates random number in (0,1)...*/
	randomNumber=rand ()/(RAND_MAX+1.0);		
	value=randomNumber*0xffffffff;
	/*fetches initial time...*/
	gettimeofday (&tvBeginUnprotWrite, NULL);
	/*performs N writes to unprot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*LWP_GetPageSize ()/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		unprot_mem[index]=value;
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndUnprotWrite, NULL);
	unprot_write_times=tvEndUnprotWrite.tv_sec+(tvEndUnprotWrite.tv_usec*0.000001)
			-tvBeginUnprotWrite.tv_sec-(tvBeginUnprotWrite.tv_usec*0.000001);		

	printf ("Protected read loop ");
	fflush (stdout);
	/*fetches initial time...*/
	gettimeofday (&tvBeginProtRead, NULL);
	/*performs N reads from prot mem...*/
	for (i=0; i<N; i++)
	{	
		
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*LWP_GetPageSize ()/4;
		//printf ("Index=%d\n", index);
	
		/*performs access...*/
		value=prot_mem[index];	
		
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndProtRead, NULL);
	prot_read_times=tvEndProtRead.tv_sec+(tvEndProtRead.tv_usec*0.000001)
			-tvBeginProtRead.tv_sec-(tvBeginProtRead.tv_usec*0.000001);
				
	printf ("Unprotected read loop ");
	fflush (stdout);
	/*fetches initial time...*/
	gettimeofday (&tvBeginUnprotRead, NULL);
	/*performs N reads from unprot mem...*/
	for (i=0; i<N; i++)
	{	
		/*generates random number in (0,1)...*/
		randomNumber=rand ()/(RAND_MAX+1.0);
		
		index=randomNumber*NUM_PAGES*LWP_GetPageSize ()/4;
		//printf ("Index=%d\n", index);
		
		/*performs access...*/
		value=unprot_mem[index];
			
		/*if (i%(N/10)==0)
		{
			//printf ("%d ", i);
			//fflush (stdout);
		}*/
	}
	printf ("\n");
	/*fetches final time...*/
	gettimeofday (&tvEndUnprotRead, NULL);
	unprot_read_times=tvEndUnprotRead.tv_sec+(tvEndUnprotRead.tv_usec*0.000001)
			-tvBeginUnprotRead.tv_sec-(tvBeginUnprotRead.tv_usec*0.000001);
	
	
	/*prints results...*/
	
	printf ("Seconds taken for %d protected writes: %f\n", N, prot_write_times);
	
	printf ("Seconds taken for %d unprotected writes: %f\n", N, unprot_write_times);
	
	printf ("Seconds taken for %d protected reads: %f\n", N, prot_read_times);
	
	printf ("Seconds taken for %d unprotected reads: %f\n", N, unprot_read_times);
#endif	
	
	/*clean up...*/
	rfree ((void*)prot_mem);
	free ((void*)unprot_mem);
	
	return 0;
}
