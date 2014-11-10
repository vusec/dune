CC = gcc
LD = ld
CFLAGS  = -Wall -O2 -MD -I ../.. -I ../sandbox
LDFLAGS = -O2 -L ../../libdune -L ../sandbox -lsandbox -ldune -lpthread -T ../sandbox/sandbox.ld

all: multivar

multivar: multivar.o ../sandbox/libsandbox.a ../../libdune/libdune.a
	$(CC) -o $(@) $(<) $(LDFLAGS)

../sandbox/libsandbox.a:
	make -C ../sandbox libsandbox.a
../../libdune/libdune.a:
	make -C ../../libdune libdune.a

clean:
	rm -f *.o *.d multivar

-include *.d
