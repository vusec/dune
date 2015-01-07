#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

void main()
{
    int id = syscall(SYS_gettid);
    printf("gettid: %d\n", id);
}
