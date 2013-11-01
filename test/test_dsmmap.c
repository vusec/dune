#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include <libdune/dune.h>

int dsmmap_init()
{
    int ret;

    ret = dune_init_and_enter();
    if (ret) {
        printf("failed to initialize dune\n");
        return ret;
    }

    return 0;
}

#define TDS_LOOP_ITER  3
#define TDS_BUFF_PAGES 4

char *buffer = NULL;

void tds_init()
{
    int ret = dsmmap_init();
    assert(ret == 0);

    buffer = mmap(NULL, PGSIZE*TDS_BUFF_PAGES, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(buffer != MAP_FAILED);
}

void tds_top_of_the_loop(int iteration)
{
    printf("tds: top of the loop, iteration: %d\n", iteration);
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
    munmap(buffer, PGSIZE*TDS_BUFF_PAGES);
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

    return 0;
}
