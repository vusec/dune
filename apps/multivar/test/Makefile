CC = gcc
LD = ld
CFLAGS  = -Wall -O2 -MD -g

# For LD_PRELOAD tests
LDFLAGS = -rdynamic -L ../../../libdune -Wl,--whole-archive -ldune -Wl,--no-whole-archive -T ../../../libdune/libdune.ld
# For multivar-sandbox tests:
#LDFLAGS =

all: gettid getpid getpid_bench getpid_bench2 fork getcpu

gettid: gettid.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
getpid: getpid.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
getcpu: getcpu.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
getpid_bench: getpid_bench.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
getpid_bench2: getpid_bench2.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
fork: fork.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)


clean:
	rm -f *.o *.d gettid getpid getpid_bench getpid_bench2 fork getcpu

-include *.d
