#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define USE_THREADS 0 // otherwise uses signals

// timer globals
timer_t         global_timer;
struct itimerspec      timer_burst;
struct itimerspec      timer_no_burst;
unsigned short  burst_on;   // enable or disable bursting
unsigned long   sampling_rate;
unsigned long   burst_length;

/* Per capire come sono fatte
struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct itimerspec {
    struct timespec it_interval;  // Timer interval
    struct timespec it_value;     // Initial expiration
};*/

#if USE_THREADS
void timer_handler(sigval_t val)
#else
void timer_handler(int x, siginfo_t* y, void* z)
#endif
{
        if (burst_on!=0) {
            printf("[TIMER] time elapsed: %ld nanoseconds\n", timer_burst.it_interval.tv_nsec);
            burst_on=0;
            timer_settime(global_timer, 0, &timer_no_burst, NULL);
        } else {
            printf("[TIMER] time elapsed: %ld nanoseconds\n", timer_no_burst.it_interval.tv_nsec);
            burst_on=1;
            timer_settime(global_timer, 0, &timer_burst, NULL);
        }
}


int main(int argc, char** argv) {
     // Initializing timer (granularity is nanoseconds from Linux 2.6.21 - hrtimer)
        burst_on=1;
        sampling_rate=10*1000000;
        burst_length=1*1000000;
        
        timer_burst.it_value.tv_sec = 0;
        timer_burst.it_value.tv_nsec = burst_length;
        timer_burst.it_interval.tv_sec = 0;
        timer_burst.it_interval.tv_nsec = burst_length;
        
        timer_no_burst.it_value.tv_sec = 0;
        timer_no_burst.it_value.tv_nsec = sampling_rate-burst_length;
        timer_no_burst.it_interval.tv_sec = 0;
        timer_no_burst.it_interval.tv_nsec = sampling_rate-burst_length;
        
        struct sigevent evp = {0}; // zero all members
        
        #if USE_THREADS
        evp.sigev_notify = SIGEV_THREAD;
        evp.sigev_notify_function = timer_handler;
        //~ evp.sigev_value.sival_ptr = (void*)&burst_on; // faster?
        #else
        evp.sigev_notify = SIGEV_SIGNAL;
        evp.sigev_signo = SIGPROF;        
        
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = timer_handler;
        // set up sigaction to catch signal
        if (sigaction(SIGPROF, &sa, NULL) == -1) {
            perror("sa failed");
            exit( EXIT_FAILURE );
        }        
        #endif
        
        int ret;
        ret=timer_create(CLOCK_PROCESS_CPUTIME_ID, &evp, &global_timer);
        if (ret) {
            printf("timer_create error - exiting!\n");
            exit(1);
        }
        ret=timer_settime(global_timer, 0, &timer_burst, NULL);
        if (ret) {
            printf("timer_settime error - exiting!\n");
            exit(1);
        }
        
        // PROBLEMA: IL TIMER VIENE PERSO!!! (non posso usarlo dentro constructor gcc)
        while(1); // unico modo per farlo funzionare... mi serve un altro thread :(
                    
    return 0;   
}

/*Ispirato a http://stackoverflow.com/questions/6094760/posix-timer-have-multiple-timers

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <linux/socket.h>
#include <time.h>


#define SIGTIMER (SIGRTMAX)
#define SIG SIGUSR1
static timer_t     tid;
static timer_t     tid2;

void SignalHandler(int, siginfo_t*, void* );
timer_t SetTimer(int, int, int);

int main(int argc, char *argv[]) {


    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = SignalHandler;
    // set up sigaction to catch signal
    if (sigaction(SIGTIMER, &sigact, NULL) == -1) {
        perror("sigaction failed");
        exit( EXIT_FAILURE );
    }

    // Establish a handler to catch CTRL+c and use it for exiting.
    sigaction(SIGINT, &sigact, NULL);
    tid=SetTimer(SIGTIMER, 1000, 1);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = SignalHandler;
    // set up sigaction to catch signal
    if (sigaction(SIG, &sa, NULL) == -1) {
        perror("sa failed");
        exit( EXIT_FAILURE );
    }

    // Establish a handler to catch CTRL+c and use it for exiting.
    sigaction(SIGINT, &sa, NULL);
    tid2=SetTimer(SIG, 1000, 3);
    for(;;);
    return 0;
}

void SignalHandler(int signo, siginfo_t* info, void* context)
{
    if (signo == SIGTIMER) {
        printf("Command Caller has ticked\n");

    }else if (signo == SIG) {
        printf("Data Caller has ticked\n");

    } else if (signo == SIGINT) {
        timer_delete(tid);
        perror("Crtl+c cached!");
        exit(1);  // exit if CRTL/C is issued
    }
}
timer_t SetTimer(int signo, int sec, int mode)
{
    static struct sigevent sigev;
    static timer_t tid;
    static struct itimerspec itval;
    static struct itimerspec oitval;

    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = signo;
    sigev.sigev_value.sival_ptr = &tid;

    if (timer_create(CLOCK_REALTIME, &sigev, &tid) == 0) {
        itval.it_value.tv_sec = sec / 1000;
        itval.it_value.tv_nsec = (long)(sec % 1000) * (1000000L);

        if (mode == 1) {
            itval.it_interval.tv_sec = itval.it_value.tv_sec;
            itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
        }
        else {
            itval.it_interval.tv_sec = 0;
            itval.it_interval.tv_nsec = 0;
        }

        if (timer_settime(tid, 0, &itval, &oitval) != 0) {
            perror("time_settime error!");
        }
    }
    else {
        perror("timer_create error!");
        return NULL;
    }
    return tid;
}
*/
