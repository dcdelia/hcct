# ======================================================================
#  Makefile
# ======================================================================

#  Author:         (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
#  License:        See the end of this file for license information
#  Created:        November 4, 2010
#  Module:         hcct

#  Last changed:   $Date: 2011/03/15 22:18:10 $
#  Changed by:     $Author: pctips $
#  Revision:       $Revision: 1.5 $


# defines
CC 	        = gcc
SRC_DIR     = src/
BIN_DIR     = bin/
LOG_DIR     = logs/
INCLUDE     = -I ../dc/test                               \
              -I ../dc/include                            \
              -I include
COMMON_SRC 	= ../dc/src/pool.c                            \
              ../dc/test/profile.c                        \
              ../dc/test/logfile.c                        \
              $(SRC_DIR)driver.c                             \
              $(SRC_DIR)metrics.c                            \
              $(SRC_DIR)burst.c                              \
              $(SRC_DIR)common.c							 \
              -lm
CFLAGS 		= -D_LARGEFILE_SOURCE                            \
              -D_LARGEFILE64_SOURCE                          \
              -D_FILE_OFFSET_BITS=64                         \
              -Wall                                          \
              -O3


# default test parameters
PHI 	 = 10000
EPSILON  = 50000
TAU 	 = 10
SAMPLING = 50
BURST    = 1
TRACES   = amarok ark firefox oocalc inkscape
RR		 = 10


# ----------------------------------------------------------------------
#  build rules
# ----------------------------------------------------------------------
all: empty cct lss-hcct bss-hcct lc-hcct

# build empty test drivers
empty:
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty                         \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE)
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty-burst                   \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-10             \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=10		   \
	      -DEMPTY=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-20             \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=20		   \
	      -DEMPTY=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-5              \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=5 		   \
	      -DEMPTY=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-no-RR          \
	      $(SRC_DIR)empty.c                                    \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=RAND_MAX     \
	      -DEMPTY=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-1		       \
	      #~ $(SRC_DIR)empty.c                                    \
	      #~ $(COMMON_SRC) $(INCLUDE) 			                   \
	   	  #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=1			   \
	   	  #~ -DEMPTY=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)empty-adaptive-wc             \
	      #~ $(SRC_DIR)empty.c                                    \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=$(RR)		   \
	      #~ -DEMPTY=1 -DWEIGHT_COMPENSATION=1

# build cct test drivers
cct:
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct                           \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-size-hcct                 \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)	      					   \
	      -DCOMPUTE_HOT_PER_PHI=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-max-phi                   \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPUTE_MAX_PHI=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-burst                     \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-10               \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=10
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-20               \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=20
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-5                \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=5
	$(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-no-RR            \
	      $(SRC_DIR)cct.c                                      \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=RAND_MAX
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-1                \
	      #~ $(SRC_DIR)cct.c                                      \
	      #~ $(COMMON_SRC) $(INCLUDE) 			                   \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)cct-adaptive-wc               \
	      #~ $(SRC_DIR)cct.c                                      \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=$(RR)        \
	      #~ -DWEIGHT_COMPENSATION=1

# build lss-hcct test drivers
lss-hcct:
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-perf                 \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE)
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-burst-perf           \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-10-perf     \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=10
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-20-perf     \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=20
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-5-perf      \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=5
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-no-RR-perf  \
	      $(SRC_DIR)lss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=RAND_MAX
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-1-perf      \
	      #~ $(SRC_DIR)lss-hcct.c                                 \
	      #~ $(COMMON_SRC) $(INCLUDE) 			                   \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-wc-perf     \
	      #~ $(SRC_DIR)lss-hcct.c                                 \
	      #~ $(COMMON_SRC) $(INCLUDE) 			                   \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=$(RR)		   \
	      #~ -DWEIGHT_COMPENSATION=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-metrics              \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-burst-metrics        \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-10-metrics  \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=10
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-20-metrics  \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=20
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-5-metrics   \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=5
	$(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-no-RR-metrics \
	      $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=RAND_MAX
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-eps-metrics          \
	      #~ $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DCOMPARE_TO_CCT=1 -DLSS_ADJUST_EPS=1
	      
    #~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-10-metrics-wc  \
	      #~ $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      #~ -DRE_ENABLING=10 -DWEIGHT_COMPENSATION=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-20-metrics-wc  \
	      #~ $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      #~ -DRE_ENABLING=20 -DWEIGHT_COMPENSATION=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lss-hcct-adaptive-5-metrics-wc \
	      #~ $(SRC_DIR)lss-hcct.c $(SRC_DIR)cct.c                 \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      #~ -DRE_ENABLING=5 -DWEIGHT_COMPENSATION=1	      

# build bss-hcct test driver (for performance only)
bss-hcct:
	$(CC) $(CFLAGS) -o $(BIN_DIR)bss-hcct-perf                 \
	      $(SRC_DIR)bss-hcct.c                                 \
	      $(COMMON_SRC) $(INCLUDE)

# build lc-hcct test drivers
lc-hcct:
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-perf                  \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE)
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-burst-perf            \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-10-perf      \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=10
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-20-perf      \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=20
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-5-perf       \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=5
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-no-RR-perf   \
	      $(SRC_DIR)lc-hcct.c                                  \
	      $(COMMON_SRC) $(INCLUDE) 			                   \
	      -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=RAND_MAX	
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-1-perf       \
	      #~ $(SRC_DIR)lc-hcct.c                                  \
	      #~ $(COMMON_SRC) $(INCLUDE) 			                   \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=1
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-wc-perf      \
	      #~ $(SRC_DIR)lc-hcct.c                                  \
	      #~ $(COMMON_SRC) $(INCLUDE)                             \
	      #~ -DBURSTING=1 -DADAPTIVE=1 -DRE_ENABLING=$(RR)        \
	      #~ -DWEIGHT_COMPENSATION=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-metrics               \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	      $(COMMON_SRC) $(INCLUDE)                             \
	      -DCOMPARE_TO_CCT=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-burst-metrics         \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       $(COMMON_SRC) $(INCLUDE)                            \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-10-metrics   \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       $(COMMON_SRC) $(INCLUDE)                            \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=10
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-20-metrics   \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       $(COMMON_SRC) $(INCLUDE)                            \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=20
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-5-metrics    \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       $(COMMON_SRC) $(INCLUDE)                            \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=5
	$(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-no-RR-metrics \
	      $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       $(COMMON_SRC) $(INCLUDE)                            \
	      -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      -DRE_ENABLING=RAND_MAX
	#~ $(CC) $(CFLAGS) -o $(BIN_DIR)lc-hcct-adaptive-wc-metrics   \
	      #~ $(SRC_DIR)lc-hcct.c $(SRC_DIR)cct.c                  \
	       #~ $(COMMON_SRC) $(INCLUDE)                            \
	      #~ -DCOMPARE_TO_CCT=1 -DBURSTING=1 -DADAPTIVE=1		   \
	      #~ -DRE_ENABLING=$(RR) -DWEIGHT_COMPENSATION=1

tonight: lss-hcct-burst-metrics-test-phi


# ----------------------------------------------------------------------
#  empty tests
# ----------------------------------------------------------------------

# exhaustive - performance test
empty-test: $(BIN_DIR)empty
	cd scripts ;                                               \
	perl do-for-all-traces.pl empty

# static bursting - performance test
empty-burst-test: $(BIN_DIR)empty-burst
	cd scripts ;                                               \
	perl do-for-all-traces.pl empty-burst					   \
		     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)



# ----------------------------------------------------------------------
#  cct tests
# ----------------------------------------------------------------------

# cct test (performance + general statistics)
cct-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl cct

# cct test (performance + general statistics)
cct-test-max-phi:
	cd scripts ;                                               \
	perl do-for-all-traces-max-phi.pl cct-max-phi

# cct-burst test (performance + general statistics)
cct-burst-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl cct-burst                        \
	     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)


# ----------------------------------------------------------------------
#  lss-hcct tests
# ----------------------------------------------------------------------

# exhaustive - performance test
lss-hcct-perf-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl lss-hcct-perf                    \
	     $(PHI) $(EPSILON) $(TAU)

# static bursting  - performance test
lss-hcct-burst-perf-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl lss-hcct-burst-perf              \
	     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)

# exhaustive - metrics test
lss-hcct-metrics-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl lss-hcct-metrics                 \
	     $(PHI) $(EPSILON) $(TAU)
	     
# exhaustive - metrics test with eps field in nodes
lss-hcct-eps-metrics-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl lss-hcct-eps-metrics             \
	     $(PHI) $(EPSILON) $(TAU)	     

# static bursting - metrics test
lss-hcct-burst-metrics-test:
	cd scripts ;                                               \
	perl do-for-all-traces.pl lss-hcct-burst-metrics           \
	     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)

# exhaustive - metrics test for all phi
lss-hcct-metrics-test-phi:
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-phi.pl lss-hcct-metrics $$trace ;      \
	done

# exhaustive - metrics test for all epsilon
lss-hcct-metrics-test-epsilon:
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-epsilon.pl lss-hcct-metrics $$trace ;  \
	done

# static bursting - metrics test for all phi
lss-hcct-burst-metrics-test-phi:
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-phi.pl lss-hcct-burst-metrics $$trace  \
	        $(SAMPLING) $(BURST) ;                             \
	done


# ----------------------------------------------------------------------
#  bss-hcct tests
# ----------------------------------------------------------------------

# exhaustive - performance test
bss-hcct-perf-test: $(BIN_DIR)bss-hcct-perf
	cd scripts ;                                               \
	perl do-for-all-traces.pl bss-hcct-perf                    \
	$(PHI) $(EPSILON) $(TAU)


# ----------------------------------------------------------------------
#  lc-hcct tests
# ----------------------------------------------------------------------

# exhaustive - performance test
lc-hcct-perf-test: $(BIN_DIR)lc-hcct-perf
	cd scripts ;                                               \
	perl do-for-all-traces.pl lc-hcct-perf                     \
	     $(PHI) $(EPSILON) $(TAU)

# static bursting  - performance test
lc-hcct-burst-perf-test: $(BIN_DIR)lc-hcct-burst-perf
	cd scripts ;                                               \
	perl do-for-all-traces.pl lc-hcct-burst-perf               \
	     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)

# exhaustive - metrics test
lc-hcct-metrics-test: $(BIN_DIR)lc-hcct-metrics
	cd scripts ;                                               \
	perl do-for-all-traces.pl lc-hcct-metrics                  \
	     $(PHI) $(EPSILON) $(TAU)

# static bursting - metrics test
lc-hcct-burst-metrics-test: $(BIN_DIR)lc-hcct-burst-metrics
	cd scripts ;                                               \
	perl do-for-all-traces.pl lc-hcct-burst-metrics            \
	     $(PHI) $(EPSILON) $(TAU) $(SAMPLING) $(BURST)

# exhaustive - metrics test for all phi
lc-hcct-metrics-test-phi: $(BIN_DIR)lc-hcct-metrics
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-phi.pl lc-hcct-metrics $$trace ;       \
	done

# exhaustive - metrics test for all epsilon
lc-hcct-metrics-test-epsilon:
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-epsilon.pl lc-hcct-metrics $$trace ;   \
	done

# static bursting - metrics test for all phi
lc-hcct-burst-metrics-test-phi:
	cd scripts ;                                               \
	for trace in $(TRACES) ; do                                \
	    perl do-for-all-phi.pl lc-hcct-burst-metrics $$trace   \
	        $(SAMPLING) $(BURST) ;                             \
	done


# ----------------------------------------------------------------------
#  clean rules
# ----------------------------------------------------------------------

clean:
	rm -rf $(BIN_DIR)*

clean-logs:
	rm -rf $(LOG_DIR)*


# Copyright (C) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  
#  USA 
