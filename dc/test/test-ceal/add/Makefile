CEAL_DIR = ../../ceal-0.1.1/
INCLUDE_DIR = -I$(CEAL_DIR)/src/runtime/ -I$(CEAL_DIR)/src/apps/ -I../..
CEAL_CC = $(CEAL_DIR)/bin/cealc
CC = gcc
DRIVER_FLAGS = -D CONV=0 -D CHECK=0

all: add

add:
	$(CC) $(DRIVER_FLAGS) -c ../../test-driver.c ../../profile.c ../../logfile.c
	$(CEAL_CC) $(DRIVER_FLAGS) ./add.c ./add-test-ceal.c $(CEAL_DIR)/src/apps/pair.c $(CEAL_DIR)/src/apps/coin.c \
		$(CEAL_DIR)/src/apps/conscell.c $(CEAL_DIR)/src/apps/modlist.c $(CEAL_DIR)/src/apps/reduce.c \
		logfile.o profile.o test-driver.o -o add-test-ceal $(INCLUDE_DIR)
	rm -f ./profile.o ./test-driver.o ./logfile.o

add-test-rnd:
	rm -f add-test-rnd-ceal.log
	for seq_len in 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 ; do \
    ./add-test-ceal 100000 "rnd-$$seq_len" add-test-rnd-ceal.log 971 1; \
    done

add-test:
	#rm -f add-test-ceal-O3.log
	for input_size in 100000 200000 300000 400000 500000 600000 700000 800000 900000 ; do \
    ./add-test-ceal $$input_size "n/a" add-test-ceal-O3.log 971 1; \
    done
    
add-test-final:
	./add-test-ceal 100000 "n/a" add-final-test-ceal-O3.log 971 5
