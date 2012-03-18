CEAL_DIR = ../../ceal-0.1.1/
INCLUDE_DIR = -I$(CEAL_DIR)/src/runtime/ -I$(CEAL_DIR)/src/apps/ -I../..
CEAL_CC = $(CEAL_DIR)/bin/cealc
CC = gcc
DRIVER_FLAGS = -D CONV=0 -D CHECK=0

all: mergesort

mergesort:
	$(CC) $(DRIVER_FLAGS) -c ../../test-driver.c ../../profile.c ../../logfile.c
	$(CEAL_CC) $(DRIVER_FLAGS) ./_mergesort.c ./mergesort-test-ceal.c $(CEAL_DIR)/src/apps/pair.c \
		$(CEAL_DIR)/src/apps/conscell.c $(CEAL_DIR)/src/apps/modlist.c $(CEAL_DIR)/src/apps/mergesort.c \
		$(CEAL_DIR)/src/apps/coin.c \
		logfile.o profile.o test-driver.o -o mergesort-test-ceal $(INCLUDE_DIR)
	rm -f ./profile.o ./test-driver.o ./logfile.o

mergesort-test:
	rm -f mergesort-test-ceal.log
	for input_size in 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000 ; do \
    ./mergesort-test-ceal $$input_size "n/a" mergesort-test-ceal.log 971 1; \
    done
    
mergesort-test-k1:
	rm -f mergesort-test-k1-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./mergesort-test-ceal $$input_size "k=1" mergesort-test-k1-ceal.log 971 1; \
    done
    
mergesort-test-k2:
	rm -f mergesort-test-k2-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./mergesort-test-ceal $$input_size "k=2" mergesort-test-k2-ceal.log 971 1; \
    done
    
mergesort-test-k4:
	rm -f mergesort-test-k4-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./mergesort-test-ceal $$input_size "k=4" mergesort-test-k4-ceal.log 971 1; \
    done
    
mergesort-test-k8:
	rm -f mergesort-test-k8-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./mergesort-test-ceal $$input_size "k=8" mergesort-test-k8-ceal.log 971 1; \
    done
