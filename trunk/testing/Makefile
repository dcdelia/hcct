# Very simple makefile
CC  =		gcc
OPTS =		-finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread
WRAP =		-Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit
INSTR =		-finstrument-functions -lpthread $(WRAP)

all:	libs test

libs:	Makefile cct.c cct.h lss-hcct.c lss-hcct.h empty.c empty.h common.h allocator/pool.c allocator/pool.h
		$(CC) -c cct.c
		$(CC) -c cct.c -DUSE_MALLOC -o cct_malloc.o
		$(CC) -c empty.c
		$(CC) -c lss-hcct.c
		$(CC) -c allocator/pool.c
		## Devo sistemare queste schifezze dei flag e degli include...
		# empty profiler
		$(CC) -c core_profiler.c -DPROFILER_EMPTY
		$(CC) -shared empty.o core_profiler.o -o libhcct_empty.so $(WRAP)
		# cct profiler(s)
		$(CC) -c core_profiler.c -DPROFILER_CCT
		$(CC) -shared cct.o core_profiler.o pool.o -o libhcct_cct.so $(WRAP)
		$(CC) -shared cct_malloc.o core_profiler.o pool.o -o libhcct_cct_malloc.so $(WRAP)
		# lss-hcct profiler
		$(CC) -c core_profiler.c
		$(CC) -shared lss-hcct.o core_profiler.o pool.o -o libhcct_lss.so $(WRAP)
		mkdir -p libs
		mv *.so libs/
		rm -f *.o
		@echo "Remember to execute once: export LD_LIBRARY_PATH=\$$PWD/libs:\$$LD_LIBRARY_PATH"

test:	Makefile example.c libs/*.so
		mkdir -p bin
		$(CC) -o bin/test_empty example.c -L./libs -lhcct_empty $(INSTR)
		$(CC) -o bin/test_cct example.c -L./libs -lhcct_cct $(INSTR)
		$(CC) -o bin/test_cct_malloc example.c -L./libs -lhcct_cct_malloc $(INSTR)
		$(CC) -o bin/test_lss example.c -L./libs -lhcct_lss $(INSTR)
		@echo "Remember to execute once: export LD_LIBRARY_PATH=\$$PWD/libs:\$$LD_LIBRARY_PATH"

clean:	
		rm -rf libs bin

#cct:	cct.c cct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
		#$(CC) -o cct core_profiler.c cct.c example.c allocator/pool.c -DPROFILER_CCT $(OPTS) 
		#$(CC) -o cct_malloc core_profiler.c cct.c example.c -DUSE_MALLOC -DPROFILER_CCT $(OPTS)
		
#lss-hcct:	lss-hcct.c lss-hcct.h core_profiler.c example.c allocator/pool.c allocator/pool.h common.h
			#$(CC) -o lss-hcct core_profiler.c lss-hcct.c example.c allocator/pool.c $(OPTS)

#empty:	core_profiler.c example.c empty.c empty.h common.h
		#$(CC) -o empty core_profiler.c empty.c example.c $(OPTS) -DPROFILER_EMPTY
