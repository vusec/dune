CC = gcc
LD = ld
CFLAGS  = -Wall -O2 -MD -I ../.. -I ../sandbox
LDFLAGS = -O2 -L ../../libdune -L ../sandbox -lsandbox -ldune -lpthread -T ../sandbox/sandbox.ld
obj-m += lkmmultivar.o
lkmmultivar-objs = kernmod2.o util.o

all: multivar multivar_intercept ptrace multivar.ko multivar_preload.so multivar_intercept_preload.so

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
	rm -f *.o *.d multivar ptrace
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


-include *.d
