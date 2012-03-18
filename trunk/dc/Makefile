# Makefile

#LL = ../ll-core
RM = ../rm
FLAGS = -m32 -O3 -fomit-frame-pointer -D DC_STAT=0
#FLAGS = -m32 -O3 -pg -D DC_STAT=1
#FLAGS = -m32 -O0 -g -D DC_STAT=1

all: liblcp

liblcp:
	gcc $(FLAGS) -c ./src/pool.c ./src/LScheduler.c ./src/dc.c  -I./include/ -I$(RM)/include/ -L$(RM)/lib/ 
	ar rcs ./lib/libdc.a ./*.o
	rm -f ./*.o

# use this if compiling with priority scheduler
#liblcp:
#	gcc $(FLAGS) -c ./src/pool.c ./src/LScheduler.c ./src/dc.c $(LL)/src/*.c -I$(LL)/include/ -I./include/ -I$(RM)/include/ -L$(RM)/lib/ 
#	ar rcs ./lib/libdc.a ./*.o
#	rm -f ./*.o

# Copyright (C) 2009 Andrea Ribichini

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
