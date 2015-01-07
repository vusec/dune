#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define NUM_CALLS 100000000

inline unsigned long long int rdtsc(void)
{
    unsigned a, d;

    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));

    return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

void main()
{
    int i;
    int id;
    unsigned long long int total, min, start, end;

    min = 0;
    total = 0;

    for (i = 0; i < NUM_CALLS; i++)
    {
        start = rdtsc();
        id = syscall(SYS_getpid);
        end = rdtsc();

        if (min == 0 || end - start < min)
            min = end - start;
        total += end - start;
    }

    printf("getpid cycles: min %llu  avg %llu\n", min, total / NUM_CALLS);

}
