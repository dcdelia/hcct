# ============================================================================
#  dc/test/Makefile
# ============================================================================

#  Author:         (c) 2010 Camil Demetrescu, Irene Finocchi, Andrea Ribichini
#  License:        See the end of this file for license information
#  Created:        October 23, 2010
#  Module:         dc/test

#  Last changed:   $Date: 2010/11/16 22:47:23 $
#  Changed by:     $Author: demetres $
#  Revision:       $Revision: 1.38 $


LL = ../../ll-core

CC = gcc
C++ = g++
RM = ../../rm
OUT_DIR = ../bin/
LIB     = -L../lib -L$(RM)/lib/ -ldisasm -lm -ldc -lrm -lelf -lglib-2.0
LIB-EXP     = -L../lib -L$(RM)/lib/ -Wl,-Bstatic -lQtUiTools -lncurses -Wl,-Bdynamic -lelf -ldisasm -lm -ldc -lrm -lglib-2.0
LIB-PRI = -L../lib -L$(RM)/lib/ -ldisasm -lm -ldc-pri -lrm -lelf -lglib-2.0
INC = -I../include -I$(RM)/include/ -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
TEST_SRC = profile.c
FLAGS_O0 = -m32 -g -O0 -Wall
FLAGS_O3 = -m32 -O3


all: test00 mem-perf2 exptrees halver mapper splitter vartracker

test-pool:
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)test-pool test-pool.c ../src/pool.c -I../include

test00:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)test00 test00.c ./dumpster/*.o $(LL)/src/*.c -I$(LL)/include/ $(INC) $(LIB)
	
mem-perf2:
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)mem-perf2 mem-perf2.c $(INC) $(LIB)

exptrees:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)exptrees-O0 exptrees.c $(TEST_SRC) $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)exptrees-O3 exptrees.c $(TEST_SRC) $(INC) $(LIB)

exptrees-cpp:
	$(C++) $(FLAGS_O0) -o $(OUT_DIR)exptrees-cpp-O0 exptrees.cpp $(TEST_SRC) $(INC) $(LIB)
	$(C++) $(FLAGS_O3) -o $(OUT_DIR)exptrees-cpp-O3 exptrees.cpp $(TEST_SRC) $(INC) $(LIB)

# obsolete
merger:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)merger-O0 merger-testapp.c merger.c rlist.c list.c $(INC) $(LIB) -D DEBUG=0
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)merger-O3 merger-testapp.c merger.c rlist.c list.c $(INC) $(LIB) -D DEBUG=0

# obsolete
mergesort-array:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)mergesort-array-O0 mergesort-array.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)mergesort-array-O3 mergesort-array.c $(INC) $(LIB)

# obsolete
shadower:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)shadower-O0 shadower-testapp.c shadower.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)shadower-O3 shadower-testapp.c shadower.c rlist.c $(INC) $(LIB)

halver:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)halver-O0 halver-testapp.c halver.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)halver-O3 halver-testapp.c halver.c rlist.c $(INC) $(LIB)

mapper:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)mapper-O0 mapper-testapp.c mapper.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)mapper-O3 mapper-testapp.c mapper.c rlist.c $(INC) $(LIB)

splitter:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)splitter-O0 splitter-testapp.c splitter.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)splitter-O3 splitter-testapp.c splitter.c rlist.c $(INC) $(LIB)

vartracker:
	$(C++) $(FLAGS_O0) -o $(OUT_DIR)vartracker-O0 vartracker.cpp $(TEST_SRC) $(INC) $(LIB)
	$(C++) $(FLAGS_O3) -o $(OUT_DIR)vartracker-O3 vartracker.cpp $(TEST_SRC) $(INC) $(LIB)

adder:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)adder-O0 adder-testapp.c adder.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)adder-O3 adder-testapp.c adder.c rlist.c $(INC) $(LIB)

joiner:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)joiner-O0 joiner-testapp.c joiner.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)joiner-O3 joiner-testapp.c joiner.c rlist.c $(INC) $(LIB)

splitter-joiner:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)splitter-joiner-O0 splitter-joiner-testapp.c splitter.c joiner.c rlist.c $(INC) $(LIB-PRI)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)splitter-joiner-O3 splitter-joiner-testapp.c splitter.c joiner.c rlist.c $(INC) $(LIB-PRI)

quicksorter:
	$(CC) $(FLAGS_O0) -o $(OUT_DIR)quicksorter-O0 \
		quicksorter-testapp.c quicksorter.c splitter.c joiner.c rlist.c $(INC) $(LIB)
	$(CC) $(FLAGS_O3) -o $(OUT_DIR)quicksorter-O3 \
		quicksorter-testapp.c quicksorter.c splitter.c joiner.c rlist.c $(INC) $(LIB)

watcher:
	$(C++) $(FLAGS_O0) -o $(OUT_DIR)watcher-O0 watcher.cpp $(TEST_SRC) $(INC) $(LIB)
	$(C++) $(FLAGS_O3) -o $(OUT_DIR)watcher-O3 watcher.cpp $(TEST_SRC) $(INC) $(LIB)


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
