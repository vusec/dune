#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/syscall.h>


int main(int argc, char **argv)
{
    pid_t pid;
    pid = fork();

    if (argc == 1)
    {
        printf("Usage: %s <program> <args>\n", argv[0]);
        return 1;
    }

    if (pid == 0)
    {
        char **program = &argv[1];
        execvp(program[0], program);
        perror("execve");
    }

    close(fileno(stdout));
    close(fileno(stderr));
    close(fileno(stdin));

    return 0;
}
