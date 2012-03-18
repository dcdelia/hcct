CEAL_DIR = ../../ceal-0.1.1/
INCLUDE_DIR = -I$(CEAL_DIR)/src/runtime/ -I$(CEAL_DIR)/src/apps/ -I../..
CEAL_CC = $(CEAL_DIR)/bin/cealc
CC = gcc
DRIVER_FLAGS = -D CONV=0 -D CHECK=0

all: exptrees

exptrees:
	$(CC) $(DRIVER_FLAGS) -c ../../test-driver.c ../../profile.c ../../logfile.c
	$(CEAL_CC) $(DRIVER_FLAGS) ./exptrees.c ./exptrees-test-ceal.c \
		logfile.o profile.o test-driver.o -o exptrees-test-ceal $(INCLUDE_DIR)
	rm -f ./profile.o ./test-driver.o ./logfile.o

exptrees-test:
	rm -f exptrees-test-ceal-O3.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./exptrees-test-ceal $$input_size "n/a" exptrees-test-ceal-O3.log 971 1; \
    done
