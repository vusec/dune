CC	= gcc
CFLAGS	= -Wall -g -MD -O2 -I ../ -fno-common
LDFLAGS  =  -fuse-ld=bfd -L ../libdune -ldune -lpthread

all: hello test timetest test_sandbox test_dsmmap

hello: hello.o ../libdune/libdune.a
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS)

timetest: timetest.o ../libdune/libdune.a
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS)

test: test.o ../libdune/libdune.a
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS)

test_sandbox: test_sandbox.o
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS)

test_dsmmap: test_dsmmap.o ../libdune/libdune.a
	$(CC) $(CFLAGS) -o $(@) $(<) $(LDFLAGS) -T ../libdune/libdune.ld

clean:
	rm -f *.o test *.d hello test test_sandbox test_dsmmap

-include *.d
