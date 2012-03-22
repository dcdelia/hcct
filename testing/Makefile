# Very simple makefile
CC  =		gcc
OPTS =		-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread

all:	cct lss-hcct

cct:	cct.c cct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
		$(CC) -o cct core_profiler.c cct.c example.c allocator/pool.c $(OPTS)
		$(CC) -o cct_malloc core_profiler.c cct.c example.c -DUSE_MALLOC $(OPTS)
		
lss-hcct:	lss-hcct.c lss-hcct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
			$(CC) -o lss-hcct core_profiler.c lss-hcct.c example.c allocator/pool.c $(OPTS)
		
