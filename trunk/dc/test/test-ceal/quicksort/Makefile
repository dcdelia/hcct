CEAL_DIR = ../../ceal-0.1.1/
INCLUDE_DIR = -I$(CEAL_DIR)/src/runtime/ -I$(CEAL_DIR)/src/apps/ -I../..
CEAL_CC = $(CEAL_DIR)/bin/cealc
CC = gcc
DRIVER_FLAGS = -D CONV=0 -D CHECK=0

all: quicksort

quicksort:
	$(CC) $(DRIVER_FLAGS) -c ../../test-driver.c ../../profile.c ../../logfile.c
	$(CEAL_CC) $(DRIVER_FLAGS) ./_quicksort.c ./quicksort-test-ceal.c $(CEAL_DIR)/src/apps/pair.c \
		$(CEAL_DIR)/src/apps/conscell.c $(CEAL_DIR)/src/apps/modlist.c $(CEAL_DIR)/src/apps/quicksort.c \
		$(CEAL_DIR)/src/apps/coin.c \
		logfile.o profile.o test-driver.o -o quicksort-test-ceal $(INCLUDE_DIR)
	rm -f ./profile.o ./test-driver.o ./logfile.o

quicksort-test:
	rm -f quicksort-test-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./quicksort-test-ceal $$input_size "n/a" quicksort-test-ceal.log 971 3; \
    done
    
quicksort-test-k1:
	rm -f quicksort-test-k1-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./quicksort-test-ceal $$input_size "k=1" quicksort-test-k1-ceal.log 971 1; \
    done
    
quicksort-test-k2:
	rm -f quicksort-test-k2-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./quicksort-test-ceal $$input_size "k=2" quicksort-test-k2-ceal.log 971 1; \
    done
    
quicksort-test-k4:
	rm -f quicksort-test-k4-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./quicksort-test-ceal $$input_size "k=4" quicksort-test-k4-ceal.log 971 1; \
    done
    
quicksort-test-k8:
	rm -f quicksort-test-k8-ceal.log
	for input_size in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 ; do \
    ./quicksort-test-ceal $$input_size "k=8" quicksort-test-k8-ceal.log 971 1; \
    done
