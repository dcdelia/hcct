# Very simple makefile
CC  =		gcc
OPTS =		-lm -finstrument-functions -Wl,-wrap,pthread_create -Wl,-wrap,pthread_exit -lpthread

all:
	$(CC) -o example $(LL) *.c $(OPTS)
