CC = gcc
LD = ld
CFLAGS  = -g -Wall -MD -I ../.. -I ../sandbox
LDFLAGS = -O2 -L ../../libdune -L ../sandbox -lsandbox -ldune -lpthread -T ../sandbox/sandbox.ld
obj-m += lkmmultivar.o
lkmmultivar-objs = kernmod2.o util.o

all: multivar multivar_intercept ptrace multivar_preload.so multivar_intercept_preload.so background ptrace_intercept lkmmultivar.ko

multivar: multivar.o ../sandbox/libsandbox.a ../../libdune/libdune.a
	$(CC) -o $(@) $(<) $(LDFLAGS)
multivar_intercept: multivar_intercept.o ../sandbox/libsandbox.a ../../libdune/libdune.a
	$(CC) -o $(@) $(<) $(LDFLAGS)

ptrace: ptrace.o
	$(CC) -o $@ $<
ptrace_intercept: ptrace_intercept.o
	$(CC) -o $@ $<
background: background.o
	$(CC) -o $@ $<

multivar_intercept.o: multivar.c
	$(CC) $(CFLAGS) -DINTERCEPT -c -o $@ $<
ptrace_intercept.o: ptrace.c
	$(CC) $(CFLAGS) -DINTERCEPT -c -o $@ $<

lkmmultivar.ko: kernmod2.c util.S
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) CFLAGS= LDFLAGS= modules


multivar_preload.so: multivar_preload.c
	$(CC) -shared -fPIC $(CFLAGS) -o $(@) $(<) -ldl
multivar_intercept_preload.so: multivar_preload.c
	$(CC) -shared -fPIC $(CFLAGS) -DINTERCEPT -o $(@) $(<) -ldl


../sandbox/libsandbox.a:
	make -C ../sandbox libsandbox.a
../../libdune/libdune.a:
	make -C ../../libdune libdune.a

clean:
	rm -f *.o *.d multivar ptrace multivar_intercept ptrace_intercept multivar_intercept_preload.so multivar_preload.so background
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


-include *.d
