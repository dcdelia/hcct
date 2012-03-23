# Very simple makefile
CC  =		gcc
OPTS =		-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread

all:	cct lss-hcct empty

cct:	cct.c cct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
		$(CC) -o cct core_profiler.c cct.c example.c allocator/pool.c -DPROFILER_CCT $(OPTS) 
		$(CC) -o cct_malloc core_profiler.c cct.c example.c -DUSE_MALLOC -DPROFILER_CCT $(OPTS)
		
lss-hcct:	lss-hcct.c lss-hcct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
			$(CC) -o lss-hcct core_profiler.c lss-hcct.c example.c allocator/pool.c $(OPTS)

empty:	core_profiler.c example.c empty.c empty.h common.h
		$(CC) -o empty core_profiler.c empty.c example.c $(OPTS) -DPROFILER_EMPTY

clean:	
		rm -rf cct cct_malloc lss-hcct empty
