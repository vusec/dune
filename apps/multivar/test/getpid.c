#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

void main()
{
    int id = syscall(SYS_getpid);
    printf("getpid: %d\n", id);
}
