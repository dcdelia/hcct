#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h> 
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#define TIMER_RESOLUTION 100000 // 100 msecs
typedef unsigned long long UINT64;

unsigned long long *tics;
int ds_shm;

void detach() {
	if (shmctl(ds_shm, IPC_RMID, NULL)!=-1) {
		printf("Shared memory deleted. Exiting...\n");
		exit(EXIT_SUCCESS);
	}
	else {
		printf("Please run \"ipcrm -m %d\". Exiting...\n", ds_shm);
		exit(EXIT_FAILURE);
	}
}

void timer() {
	while (1) {
		(*tics)++;
		usleep(TIMER_RESOLUTION);
	}
}

int main() {
	ds_shm=shmget(25437, sizeof(UINT64), IPC_CREAT|0600);
	tics=(UINT64*)shmat(ds_shm, NULL, SHM_W);
	signal(SIGINT, detach);
	signal(SIGTERM, detach);
	signal(SIGQUIT, detach);
	signal(SIGHUP, detach);
	timer();
	return 0;
}
