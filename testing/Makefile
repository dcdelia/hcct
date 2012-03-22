# Very simple makefile
CC  =		gcc
OPTS =		-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread

all:	cct.c core_profiler.c example.c
		$(CC) -o example $(LL) core_profiler.c cct.c example.c $(OPTS)
