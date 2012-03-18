# ============================================================================
#  dc/test/test-conv/Makefile
# ============================================================================

#  Author:         (c) 2010 Camil Demetrescu, Irene Finocchi, Andrea Ribichini
#  License:        See the end of this file for license information
#  Created:        October 23, 2010
#  Module:         dc/test

#  Last changed:   $Date: 2010/11/16 12:00:46 $
#  Changed by:     $Author: ribbi $
#  Revision:       $Revision: 1.6 $


CC = gcc
C++ = g++
RM = ../../../rm
OUT_DIR = ./
LIB = -L../../lib -L$(RM)/lib/ -ldc -ldisasm -lm -lrm -lelf -lglib-2.0
INC = -I../../include -I$(RM)/include/ -I.. -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
TEST_SRC = ../profile.c ../test-driver.c ../logfile.c
FLAGS_O0 = -m32 -g -O0 -D OPT=\"O0\" -D DEBUG=0 -D DC=1 -D LOGS_DIR=\"../logs/\"
FLAGS_O3 = -m32 -O3 -Wall -D OPT=\"03\" -D DEBUG=0 -D DC=1 -D LOGS_DIR=\"../logs/\"


all: mapper adder halver splitter merger exptrees

exptrees:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=1 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)exptrees-test-driver-O0 exptrees-test-conv.c exptrees.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=1 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)exptrees-test-driver-O3 exptrees-test-conv.c exptrees.c $(TEST_SRC) $(INC) $(LIB)

exptrees-test:
	rm -f exptrees-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)exptrees-test-driver-O0 $$input_size "n/a" exptrees-conv-test-O0.log 971 1; \
    $(OUT_DIR)exptrees-test-driver-O3 $$input_size "n/a" exptrees-conv-test-O3.log 971 1; \
    done

merger:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)merger-test-driver-O0 merger-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)merger-test-driver-O3 merger-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
		
merger-test:
	rm -f splitter-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)merger-test-driver-O0 $$input_size "1-2" merger-1-2-test-conv-O0.log 971 1; \
    $(OUT_DIR)merger-test-driver-O3 $$input_size "1-2" merger-1-2-test-conv-O3.log 971 1; \
    done

splitter:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)splitter-test-driver-O0 split-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)splitter-test-driver-O3 split-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)

splitter-test:
	rm -f splitter-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)splitter-test-driver-O0 $$input_size "n/a" splitter-test-conv-O0.log 971 1; \
    $(OUT_DIR)splitter-test-driver-O3 $$input_size "n/a" splitter-test-conv-O3.log 971 1; \
    done

halver:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)halver-test-driver-O0 halve-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)halver-test-driver-O3 halve-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)

halver-test:
	rm -f halver-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)halver-test-driver-O0 $$input_size "n/a" halver-test-conv-O0.log 971 1; \
    $(OUT_DIR)halver-test-driver-O3 $$input_size "n/a" halver-test-conv-O3.log 971 1; \
    done

mapper:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)mapper-test-driver-O0 map-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)mapper-test-driver-O3 map-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)

mapper-test:
	rm -f mapper-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)mapper-test-driver-O0 $$input_size "n/a" mapper-test-conv-O0.log 971 1; \
    $(OUT_DIR)mapper-test-driver-O3 $$input_size "n/a" mapper-test-conv-O3.log 971 1; \
    done
    
adder:
	$(CC) $(FLAGS_O0) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)adder-test-driver-O0 add-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -D CONV=0 -D CHECK=0 -D MAKE_ASM_DUMP=1 \
		-o $(OUT_DIR)adder-test-driver-O3 add-test-conv.c ../list.c $(TEST_SRC) $(INC) $(LIB)

adder-test:
	rm -f adder-test-*.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    $(OUT_DIR)adder-test-driver-O0 $$input_size "n/a" adder-test-conv-O0.log 971 1; \
    $(OUT_DIR)adder-test-driver-O3 $$input_size "n/a" adder-test-conv-O3.log 971 1; \
    done
    
adder-test-final:
	$(OUT_DIR)adder-test-driver-O3 1000000 "n/a" adder-final-test-conv-O3.log 971 5
    

# Copyright (C) 2010 Camil Demetrescu, Irene Finocchi, Andrea Ribichini

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
 #  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA 
