#                           -------------------------------------
#                           Dynamic all-pairs shortest paths experimental package (dsp-1.0)
#                           Makefile (c) 2002-2003 Camil Demetrescu
#                           All rights reserved.
#                           Last changed:   $Date: 2010/11/08 11:58:21 $
#                           Changed by:     $Author: demetres $
#                           Revision:       $Revision: 1.3 $

#                           -------------------------------------

#                           -------------------------------------
#                           Macros
#                           -------------------------------------
LL_PATH =                   ../ll/
C  =                        gcc -I$(LL_PATH)include -Iinclude -I../include
LL =                        $(LL_PATH)src/core/common/*.c $(LL_PATH)src/core/posix/*.c
OPTS_STD =					-lm
OPTS_INSTR =				$(OPTS_STD) -finstrument-functions -lpthread -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit
#OPTS_INSTR =				-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread
#CCT_FILES =					../../cct.c ../../core_profiler.c ../../allocator/pool.c
#LSS_HCCT_FILES = 			../../lss-hcct.c ../../core_profiler.c ../../allocator/pool.c
#EMPTY_FILES = 				../../empty.c ../../core_profiler.c

#                           -------------------------------------
#                           Rules
#                           -------------------------------------


all:						dsp instr gprof

dsp:
	                        cd bin; $(C) -o dsp $(LL) ../src/*.c $(OPTS_STD)
	                        
instr:
	                        @## Vecchio stile :P
							@#cd bin; $(C) -o dsp-lss $(LL) ../src/*.c $(LSS_HCCT_FILES) $(OPTS_INSTR)
	                        cd bin; $(C) -o dsp-cct $(LL) ../src/*.c -L../../libs -lhcct_cct $(OPTS_INSTR)
	                        cd bin; $(C) -o dsp-cct_malloc $(LL) ../src/*.c -L../../libs -lhcct_cct_malloc $(OPTS_INSTR)
	                        cd bin; $(C) -o dsp-lss $(LL) ../src/*.c -L../../libs -lhcct_lss $(OPTS_INSTR)
	                        cd bin; $(C) -o dsp-empty $(LL) ../src/*.c -L../../libs -lhcct_empty $(OPTS_INSTR)	                        
							@echo "Remember to execute once: export LD_LIBRARY_PATH=\$$PWD/libs:\$$LD_LIBRARY_PATH"							

gprof:
	                        cd bin; $(C) -pg -o dsp-gprof $(LL) ../src/*.c $(OPTS_STD)
	                        
clean:
							cd bin; rm -f *

test:
	                        cd bin; $(C) -o TestLHeap $(LL) ../test/TestLHeap.c $(OPTS_STD)

pack:
	                        $(MAKE) clean; cd ..; tar cf dsp-1.0.tar dsp-1.0; gzip dsp-1.0.tar

