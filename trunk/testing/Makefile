# Very simple makefile
CC  =		gcc
OPTS =		-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread

all:	cct.c core_profiler.c example.c allocator/pool.c allocator/pool.h cct.h
		$(CC) -o example $(LL) core_profiler.c cct.c example.c allocator/pool.c $(OPTS)
		$(CC) -o example_malloc $(LL) core_profiler.c cct.c example.c -DUSE_MALLOC $(OPTS)
		
