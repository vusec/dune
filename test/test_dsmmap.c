#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#include <libdune/dsmmap.h>

#define TDS_LOOP_ITER  3
#define TDS_BUFF_PAGES 4

char *buffer = NULL;

void tds_init()
{
    int ret;

    buffer = mmap(NULL, PGSIZE*TDS_BUFF_PAGES, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(buffer != MAP_FAILED);

    ret = dsmmap_init();
    assert(ret == 0);

    ret = dsmmap(0, ULONG_MAX);
    assert(ret == 0);
}

void tds_top_of_the_loop(int iteration)
{
    printf("tds: top of the loop, iteration: %d\n", iteration);
    dsmctl(DSMMAP_DSMCTL_CHECKPOINT, NULL);
}

void tds_loop(int iteration)
{
    int i;

    for (i=0;i<TDS_BUFF_PAGES;i++) {
        buffer[i*PGSIZE] = iteration;
    }

    sleep(1);
}

void tds_close()
{
    dsmunmap(0, ULONG_MAX);
    munmap(buffer, PGSIZE*TDS_BUFF_PAGES);
    buffer = NULL;
}

int main()
{
    int i;
    tds_init();

    for (i=0;i<TDS_LOOP_ITER;i++) {
        tds_top_of_the_loop(i);
        tds_loop(i);
    }
    tds_close();

    return 1;
}

