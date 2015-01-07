#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    pid_t pid;
    fprintf(stderr, "prefork\n");

    pid = fork();

    fprintf(stderr, "pid = %d\n", pid);

    return 0;
}
