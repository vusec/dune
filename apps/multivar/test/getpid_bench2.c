#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

void main()
{
    int i;
    int id;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    for (i = 0; i < 1000000; i++)
    {
        id = syscall(SYS_getpid);
    }
    gettimeofday(&end, NULL);

    printf("gettid time: %g\n", (end.tv_sec - start.tv_sec) +
                ((end.tv_usec / 1000000.0) - (start.tv_usec / 1000000.0)));

}
