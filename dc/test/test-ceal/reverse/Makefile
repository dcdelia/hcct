CEAL_DIR = ../../ceal-0.1.1/
INCLUDE_DIR = -I$(CEAL_DIR)/src/runtime/ -I$(CEAL_DIR)/src/apps/ -I../..
CEAL_CC = $(CEAL_DIR)/bin/cealc
CC = gcc
DRIVER_FLAGS = -D CONV=0 -D CHECK=0

all: reverse

reverse:
	$(CC) $(DRIVER_FLAGS) -c ../../test-driver.c ../../profile.c ../../logfile.c
	$(CEAL_CC) $(DRIVER_FLAGS) ./reverse.c ./reverse-test-ceal.c $(CEAL_DIR)/src/apps/pair.c \
		$(CEAL_DIR)/src/apps/conscell.c $(CEAL_DIR)/src/apps/modlist.c \
		logfile.o profile.o test-driver.o -o reverse-test-ceal $(INCLUDE_DIR)
	rm -f ./profile.o ./test-driver.o ./logfile.o

reverse-test-rnd:
	rm -f reverse-test-rnd-ceal.log
	for seq_len in 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 ; do \
    ./reverse-test-ceal 100000 "rnd-$$seq_len" reverse-test-rnd-ceal.log 971 1; \
    done

reverse-test:
	rm -f reverse-test-ceal-O3.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./reverse-test-ceal $$input_size "n/a" reverse-test-ceal-O3.log 971 1; \
    done

reverse-test-k1:
	rm -f reverse-test-k1-ceal-O3.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./reverse-test-ceal $$input_size "k=1" reverse-test-k1-ceal-O3.log 971 3; \
    done

reverse-test-k2:
	rm -f reverse-test-k2-ceal.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./reverse-test-ceal $$input_size "k=2" reverse-test-k2-ceal.log 971 3; \
    done
    
reverse-test-k4:
	rm -f reverse-test-k4-ceal.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./reverse-test-ceal $$input_size "k=4" reverse-test-k4-ceal.log 971 3; \
    done
    
reverse-test-k8:
	rm -f reverse-test-k8-ceal.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 ; do \
    ./reverse-test-ceal $$input_size "k=8" reverse-test-k8-ceal.log 971 3; \
    done    
