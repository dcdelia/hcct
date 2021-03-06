# Very simple makefile
OPT_LEVEL =	-O3
CC  =		gcc $(OPT_LEVEL)
WRAP =		-Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit
INSTR =		-finstrument-functions -lpthread $(WRAP)

all:	exhaustive burst tracer tools
		@echo
		@echo "==============================================================="
		@echo "Remember to copy/symlink the libraries from libs/ into /usr/lib"
		@echo "==============================================================="
		@echo		

burst:	libs-burst test-burst

exhaustive:	libs test

libs:	Makefile cct.c cct.h lss-hcct.c lss-hcct.h empty.c empty.h common.h pool.c pool.h
		$(CC) -c cct.c
		$(CC) -c cct.c -DUSE_MALLOC -o cct_malloc.o
		$(CC) -c empty.c
		$(CC) -c lss-hcct.c
		$(CC) -c pool.c		
		@# empty profiler
		$(CC) -c core_profiler.c -DPROFILER_EMPTY
		$(CC) -shared empty.o core_profiler.o -o libhcct_empty.so $(WRAP)
		@# cct profiler(s)
		$(CC) -c core_profiler.c -DPROFILER_CCT
		$(CC) -shared cct.o core_profiler.o pool.o -o libhcct_cct.so $(WRAP)
		$(CC) -shared cct_malloc.o core_profiler.o pool.o -o libhcct_cct_malloc.so $(WRAP)
		@# lss-hcct profiler
		$(CC) -c core_profiler.c
		$(CC) -shared lss-hcct.o core_profiler.o pool.o -o libhcct_lss.so $(WRAP)
		@mkdir -p libs
		@mv *.so libs/
		@rm -f *.o		

libs-burst:	Makefile cct.c cct.h lss-hcct.c lss-hcct.h empty.c empty.h common.h pool.c pool.h
		$(CC) -c pool.c
		$(CC) -c empty.c -DBURSTING
		$(CC) -c core_profiler_bursting.c -DPROFILER_EMPTY
		$(CC) -shared empty.o core_profiler_bursting.o -o libhcct_empty_burst.so -lrt $(WRAP)
		$(CC) -c cct.c -DBURSTING
		$(CC) -c core_profiler_bursting.c -DPROFILER_CCT		
		$(CC) -shared cct.o core_profiler_bursting.o pool.o -o libhcct_cct_burst.so -lrt $(WRAP)
		$(CC) -c lss-hcct.c -DBURSTING
		$(CC) -c core_profiler_bursting.c
		$(CC) -shared lss-hcct.o core_profiler_bursting.o pool.o -o libhcct_lss_burst.so -lrt $(WRAP)
		@mkdir -p libs
		@mv *.so libs/
		@rm -f *.o
		
test:	Makefile example.c
		@mkdir -p test
		$(CC) -o test/empty example.c -L./libs -lhcct_empty $(INSTR)
		$(CC) -o test/cct example.c -L./libs -lhcct_cct $(INSTR)
		$(CC) -o test/cct_malloc example.c -L./libs -lhcct_cct_malloc $(INSTR)
		$(CC) -o test/lss example.c -L./libs -lhcct_lss $(INSTR)
		
test-burst:	Makefile example.c
		@mkdir -p test
		$(CC) -o test/empty_burst example.c -L./libs -lhcct_empty_burst $(INSTR)
		$(CC) -o test/cct_burst example.c -L./libs -lhcct_cct_burst $(INSTR)
		$(CC) -o test/lss_burst example.c -L./libs -lhcct_lss_burst $(INSTR)
		
tracer: Makefile tracer.c tracer.h example.c		
		$(CC) -c tracer.c
		$(CC) -shared tracer.o -o libhcct_tracer.so -lrt $(WRAP)
		@mkdir -p libs
		@mv *.so libs/
		@rm -f *.o		
		@mkdir -p test
		$(CC) -o test/tracer example.c -L./libs -lhcct_tracer $(INSTR)
		
tools:	Makefile analysis.c common.h analysis.h
		@mkdir -p tools
		$(CC) -o tools/analysis analysis.c

clean:	
		rm -rf libs tools test
