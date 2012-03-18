#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define RTN_ENTER	0x0
#define RTN_EXIT	0x1
#define TIMER_TICK	0x4

#define ACCT

#ifndef O_BINARY
#define O_BINARY 0
#endif

unsigned long magicNumber, timerGranularity;
unsigned long long rtnEnterEvents, rtnExitEvents, ticks, firstTick;

void rtnEnter(unsigned long *address) {
	//printf("# Routine enter - address %lu ", *address);
}
void rtnExit() {
	//printf("# Routine exit ");
}

#ifdef ACCT
void tick() {
	//printf("# Timer tick ");
}
#else
#define tick() { } // empty
#endif

size_t analyzeBuffer(char* buf, size_t buf_size, void* timer) {
	size_t left;
	char *limit;
	// get time
	if (buf_size>5)
		limit=buf+buf_size-5;
	else
		limit=buf+buf_size; // TODO: check +5 insted of +buf_size??
		
	while (buf<limit)
		switch (*buf) {
			case RTN_ENTER:
				rtnEnter((unsigned long*)(buf+1));
				buf+=5;
				--rtnEnterEvents; // safety check
				break;
			case RTN_EXIT:
				rtnExit();
				++buf;
				--rtnExitEvents; // safety check
				break;
			case TIMER_TICK:
				tick();
				++buf;
				--ticks; // safety check
				break;
			default: exit(1);
		}
	
	if (buf_size==5) return 0;
	
	left=(size_t)((limit+5)-buf); // TODO check this!
	if (left>0)
		memcpy((limit+5-buf_size), buf, left);

	// get time + update
	
	return left;
}

int main(int argc, char* argv[]) {
	int ret, left=0, fd, buffSize=50000000;

	if (argc!=2) exit(1);
	
	char *buffer=(char*)malloc(buffSize);
	if (buffer==NULL) exit(1);
	
	fd=open(argv[1], O_RDONLY|O_BINARY);
	if (fd==-1) exit(1);
	
	ret=read(fd, buffer, 40);
	if (ret==-1) exit(1);
	
	// Header
	magicNumber=*(unsigned long*)buffer;
	rtnEnterEvents=*(unsigned long long*)(buffer+4);
	rtnExitEvents=*(unsigned long long*)(buffer+12);
	ticks=*(unsigned long long*)(buffer+20);
	firstTick=*(unsigned long long*)(buffer+28);
	timerGranularity=*(unsigned long*)(buffer+36);
	
	printf("Header: %x %llu %llu %llu %llu %lu\n", magicNumber, rtnEnterEvents, rtnExitEvents, ticks,
			firstTick, timerGranularity);
	
	do {
		ret=read(fd, buffer+left, buffSize-left);
		if (ret==-1) exit(1);
		left=analyzeBuffer(buffer, ret+left, NULL);
	} while (ret!=0 || left!=0);
	
	printf("RtnEnter, RtnExit and TimerTicks missed: %llu %llu %llu\n", rtnEnterEvents, rtnExitEvents, ticks);
	printf("Done!\n");
	exit(0);

}
