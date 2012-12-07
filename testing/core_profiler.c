#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> // syscall()
#include <asm/unistd.h> // syscall(__NR_gettid)
#include <sys/types.h> // pid_t

//~ #include <fcntl.h>
//~ #include <string.h>
//~ #include <sys/sendfile.h>
//~ #include <sys/stat.h> 


#include <string.h>
#define _GNU_SOURCE
#include <errno.h>
//extern char *program_invocation_name;
extern char *program_invocation_short_name;

// TODO
#ifdef PROFILER_CCT
#include "cct.h"
#else
#ifdef PROFILER_EMPTY
#include "empty.h"
#else
#include "lss-hcct.h"
#endif
#endif

#ifndef PROFILER_EMPTY
/* // Old version
void __attribute__ ((no_instrument_function)) hcct_dump_map() {

	int ds_in, ds_out;
	pid_t pid=syscall(__NR_getpid);
	
	struct stat stat_buf; 
	off_t offset = 0;
	
	// read from /proc/<PID>/maps
	char inputFileName[6+10+5+1]; // see dumpFileName
	sprintf(inputFileName, "/proc/%d/maps", pid);
	
	ds_in = open(inputFileName, O_RDONLY, stat_buf.st_mode);
	if (ds_in == -1) {
		printf("[profiler] WARNING: unable to read currently mapped memory regions from /proc\n");
		close(ds_in);
		return;
	} 
		
	// up to 10 digits for PID on 64 bits systems - however check /proc/sys/kernel/pid_max
	char *dumpFileName;	    
	//~ if (dumpPath==NULL) {
		dumpFileName=malloc(strlen(program_invocation_short_name)+5); // suffix: .map\0
	        sprintf(dumpFileName, "%s.map", program_invocation_short_name);
        //~ } else {
            //~ dumpFileName=malloc(strlen(dumpPath)+1+strlen(program_invocation_short_name)+5); // suffix: .map\0
	        //~ sprintf(dumpFileName, "%s/%s.map", dumpPath, program_invocation_short_name);
        //~ }
        ds_out = open(dumpFileName, O_EXCL|O_CREAT|O_WRONLY, 0660);
        if (ds_out == -1) exit((printf("[profiler] ERROR: cannot create map file %s\n", dumpFileName), 1));

    //~ }
    
    // copy
    sendfile (ds_out, ds_in, NULL, stat_buf.st_size);        
    
    free(dumpFileName);
    close(ds_in);
    close(ds_out);
	
} */

void hcct_dump_map() {
	
	pid_t pid=syscall(__NR_getpid);
		
	// Command to be executed: "cp /proc/<PID>/maps <name>.map\0" => 9+10+6+variable+5 bytes needed
	char* command=malloc(strlen(program_invocation_short_name)+30);
	sprintf(command, "cp /proc/%d/maps %s.map", pid, program_invocation_short_name);
	
	int ret=system(command);
	if (ret!=0) printf("[profiler] WARNING: unable to read currently mapped memory regions from /proc\n");	
			
}
#endif
 
// execute before main
void __attribute__ ((constructor, no_instrument_function)) trace_begin(void)
{
        #if SHOW_MESSAGES==1
        printf("[profiler] program start - tid %d\n", syscall(__NR_gettid));
        #endif
        
        if (hcct_getenv()!=0) {
            printf("[profiler] error getting parameters - exiting...\n");
            exit(1);   
        }
                
        if (hcct_init()==-1) {
            printf("[profiler] error during initialization - exiting...\n");
            exit(1);   
        }
}

// execute after termination
void __attribute__ ((destructor, no_instrument_function)) trace_end(void)
{
		#if SHOW_MESSAGES==1
        printf("[profiler] program exit - tid %d\n", syscall(__NR_gettid));
        #endif
        
        #ifndef PROFILER_EMPTY
        hcct_dump_map();
        #endif
                
        hcct_dump();
}

// Routine enter
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{		
	hcct_enter((unsigned long)this_fn, (unsigned long)call_site);
}

// Routine exit
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{	
        hcct_exit();
}

// wrap pthread_exit()
void __attribute__((no_instrument_function)) __wrap_pthread_exit(void *value_ptr)
{
		#if SHOW_MESSAGES==1
		printf("[profiler] pthread_exit - tid %d\n", syscall(__NR_gettid));
		#endif
		
		// Exit stuff
		hcct_dump();
        
        __real_pthread_exit(value_ptr);
}

// handles thread termination made without pthread_exit
void* __attribute__((no_instrument_function)) aux_pthread_create(void *arg)
{                
        hcct_init();

        // Retrieve original routine address and argument        
        void* orig_arg=((void**)arg)[1];        
        void *(*start_routine)(void*)=((void**)arg)[0];
        free(arg);
        
		// Run actual application thread's init routine
        void* ret=(*start_routine)(orig_arg);

		#if SHOW_MESSAGES==1
        printf("[profiler] return - tid %d\n", syscall(__NR_gettid));
        #endif
        
        // Exit stuff
        hcct_dump();
        
        return ret;
}

// wrap pthread_create
int __attribute__((no_instrument_function)) __wrap_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
        void** params=malloc(2*sizeof(void*));
        params[0]=start_routine;
        params[1]=arg;
        return __real_pthread_create(thread, attr, aux_pthread_create, params);
}
