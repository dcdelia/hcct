# Very simple makefile
CFLAGS =	-O3 -g
CC  =		gcc $(CFLAGS)
WRAP =		-Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit
INSTR =		-finstrument-functions -lpthread $(WRAP)

all:	exhaustive burst tools time pthread install

burst:	libs-burst test-burst

exhaustive:	libs test

pthread: Makefile common.h cct_pthread.c
		$(CC) -c cct_pthread.c
		$(CC) -shared cct_pthread.c -o libhcct_cct_pthread.so $(WRAP)
		@mv libhcct_cct_pthread.so libs/
		@rm -f *.o

time:	Makefile time.c common.h pool.c pool.h cct.c cct.h
		$(CC) -c time.c -DPROFILER_CCT
		$(CC) -c cct.c -DPROFILE_TIME		
		$(CC) -c pool.c	
		$(CC) -shared time.o cct.o pool.o -o libhcct_cct_time.so -lrt $(WRAP)		
		@mv libhcct_cct_time.so libs/
		$(CC) -c time.c
		$(CC) -c lss-hcct.c -DPROFILE_TIME
		$(CC) -shared time.o lss-hcct.o pool.o -o libhcct_lss_time.so -lrt $(WRAP)		
		@mv libhcct_lss_time.so libs/
		$(CC) -c time.c -DPROFILER_EMPTY
		$(CC) -c empty.c -DPROFILE_TIME
		$(CC) -shared time.o empty.o -o libhcct_empty_time.so -lrt $(WRAP)
		@mv libhcct_empty_time.so libs/
		@rm -f *.o
		
libs:	Makefile cct.c cct.h lss-hcct.c lss-hcct.h empty.c empty.h common.h pool.c pool.h frequency.c	
		$(CC) -c cct.c
		$(CC) -c cct.c -DUSE_MALLOC -o cct_malloc.o
		$(CC) -c empty.c
		$(CC) -c lss-hcct.c
		$(CC) -c lss-hcct.c -DUSE_MALLOC -o lss-hcct_malloc.o
		$(CC) -c pool.c		
		@# empty profiler
		$(CC) -c frequency.c -DPROFILER_EMPTY
		$(CC) -shared empty.o frequency.o -o libhcct_empty.so $(WRAP)
		@# cct profiler(s)
		$(CC) -c frequency.c -DPROFILER_CCT
		$(CC) -shared cct.o frequency.o pool.o -o libhcct_cct.so $(WRAP)
		$(CC) -shared cct_malloc.o frequency.o -o libhcct_cct_malloc.so $(WRAP)
		@# lss-hcct profiler
		$(CC) -c frequency.c
		$(CC) -shared lss-hcct.o frequency.o pool.o -o libhcct_lss.so $(WRAP)
		$(CC) -shared lss-hcct_malloc.o frequency.o pool.o -o libhcct_lss_malloc.so $(WRAP)
		@mkdir -p libs
		@mv *.so libs/
		@rm -f *.o		

libs-burst:	Makefile cct.c cct.h lss-hcct.c lss-hcct.h empty.c empty.h common.h pool.c pool.h frequency_bursting.c		
		$(CC) -c pool.c
		$(CC) -c empty.c -DBURSTING
		$(CC) -c frequency_bursting.c -DPROFILER_EMPTY
		$(CC) -shared empty.o frequency_bursting.o -o libhcct_empty_burst.so -lrt $(WRAP)
		$(CC) -c cct.c -DBURSTING
		$(CC) -c frequency_bursting.c -DPROFILER_CCT		
		$(CC) -shared cct.o frequency_bursting.o pool.o -o libhcct_cct_burst.so -lrt $(WRAP)
		$(CC) -c lss-hcct.c -DBURSTING
		$(CC) -c frequency_bursting.c
		$(CC) -shared lss-hcct.o frequency_bursting.o pool.o -o libhcct_lss_burst.so -lrt $(WRAP)
		@mkdir -p libs
		@mv *.so libs/
		@rm -f *.o
		
test:	Makefile example.c libs/
		@mkdir -p test
		$(CC) -o test/empty example.c -L./libs -lhcct_empty $(INSTR)
		$(CC) -o test/cct example.c -L./libs -lhcct_cct $(INSTR)
		$(CC) -o test/cct_malloc example.c -L./libs -lhcct_cct_malloc $(INSTR)
		$(CC) -o test/lss example.c -L./libs -lhcct_lss $(INSTR)
		
test-burst:	Makefile example.c libs/
		@mkdir -p test
		$(CC) -o test/empty_burst example.c -L./libs -lhcct_empty_burst $(INSTR)
		$(CC) -o test/cct_burst example.c -L./libs -lhcct_cct_burst $(INSTR)
		$(CC) -o test/lss_burst example.c -L./libs -lhcct_lss_burst $(INSTR)

				
tools:	Makefile analysis.c analysis.h common.h
		@mkdir -p tools
		$(CC) -o tools/analysis analysis.c -lm

install:
		@echo
		@echo "=========================================================="
		@echo "If the operations below fail, please do: sudo make install"
		@echo "=========================================================="		
		@echo
		cp -f tools/analysis /usr/local/bin/hcct_analysis
		cp libs/* /usr/local/lib
		cd /usr/local/lib && ln -f -s libhcct_cct_time.so libhcct.so		
		@echo
		@echo "====="
		@echo "Done."
		@echo "====="		
		
clean:	
		rm -rf libs tools test
