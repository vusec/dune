SHELL := /bin/bash
CC	= gcc
AR  = ar
CFLAGS	= -g -MD -O2 -I ../.. -fno-common
LDFLAGS = -L ../../libdune -ldune -lpthread
DSYMBLS = -Wl,--whole-archive -L . -lminidune -Wl,--no-whole-archive

all: dummies st_init.so libminidune.a monitor

dummies: dummy1.o dummy2.o
	$(CC) -o dummy1 dummy1.c $(LDFLAGS)
	$(CC) -o dummy2 dummy2.c $(LDFLAGS)

monitor: monitor.o libminidune.a
	$(CC) $(CFLAGS) -rdynamic -o $(@) $(<) -ldl $(DSYMBLS) $(LDFLAGS)

st_init.so: st_init.c
	$(CC) -shared -fPIC -Wall -I ../.. -o $(@) $(<)

libminidune.a: minidune.o ../../libdune/libdune.a
	$(AR) rcs $(@) $(<)

minidune.o: minidune.c
	$(CC) -c -I ../.. -o $(@) $(<) $(LDFLAGS)

.PHONY: clean dryrun

run:
	./monitor dummy1 dummy2

clean:
	rm -f *.o *.so *.d 	dummy1 dummy2 monitor libminidune.a

-include *.d

