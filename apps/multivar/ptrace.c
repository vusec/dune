#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>

typedef struct child_stat
{
    pid_t pid;
    int syscall_entered;
    unsigned long long int last_syscall;
    int override_return_value;
    unsigned long long int return_value;
} child_stat_t;

int procs_halted;

child_stat_t *find_child(child_stat_t *children, int num_children, pid_t pid)
{
    int i;
    for (i = 0; i < num_children; i++)
        if (children[i].pid == pid)
            return &children[i];
    assert(0);
}

void setup(int nproc, char **programs, child_stat_t *children)
{
    int i;
    pid_t pid;
    int status;
    struct user_regs_struct regs;


    for (i = 0; i < nproc; i++)
    {
        pid = fork();

        if (pid < 0)
            perror("fork");

        if (pid == 0)
        {
            printf("Starting %s\n", programs[i]);
            char *new_argv[] = {programs[i], NULL};
            if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
                perror("ptrace");
            execvp(new_argv[0], new_argv);
            perror("execve");
        }
        else
        {
            children[i].pid = pid;
            children[i].syscall_entered = 0;
            children[i].override_return_value = 0;

            pid_t pid2 = waitpid(pid, &status, 0);
            assert(pid == pid2);
            assert(WIFSTOPPED(status));
            ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD);
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            assert(regs.orig_rax == SYS_execve);
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
    }

}

void handle_syscall(child_stat_t *children, int nproc, pid_t pid)
{
    struct user_regs_struct regs;
    child_stat_t *child;
    int i;
    int mismatch;
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    return;

    child = find_child(children, nproc, pid);
    if (child->syscall_entered)
    {
        /* On syscall-exit, optionally override return value of syscall and then
         * resume. */
        child->syscall_entered = 0;

        if (child->override_return_value)
        {
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            regs.rax = child->return_value;
            ptrace(PTRACE_SETREGS, pid, NULL, &regs);
            child->override_return_value = 0;
        }

        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        return;
    }

    /* Barrier on syscall-enter. */

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    child->last_syscall = regs.orig_rax;
    child->syscall_entered = 1;

    procs_halted++;

    if (procs_halted < nproc)
        return;

    procs_halted = 0;

    mismatch = 0;
    for (i = 1; i < nproc; i++)
    {
        if (children[0].last_syscall != children[i].last_syscall)
        {
            printf("syscall mismatch:  %d: %llu, %d: %llu\n",
                    children[0].pid, children[0].last_syscall,
                    children[i].pid, children[i].last_syscall);
            mismatch = 1;
            break;
        }
    }
    for (i = 0; i < nproc; i++)
    {
        if (mismatch)
        {
            /* Replace the syscall by something harmless (getpid) and override
             * return value when syscall is done. */
            children[i].override_return_value = 1;
            children[i].return_value = -EPERM;
            ptrace(PTRACE_POKEUSER, children[i].pid, 8 * ORIG_RAX, SYS_getpid);
        }
        ptrace(PTRACE_SYSCALL, children[i].pid, NULL, NULL);
    }
}

int main(int argc, char **argv)
{
    const int nproc = argc - 1;
    child_stat_t *children;
    int status;
    pid_t pid;

    procs_halted = 0;
    children = malloc(sizeof(child_stat_t) * nproc);
    if (children == NULL)
        perror("malloc");

    setup(nproc, &argv[1], children);

    while (1)
    {
        if ((pid = wait(&status)) == -1)
            break;

        if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80))
        {
            handle_syscall(children, nproc, pid);
        }
        else if (WIFEXITED(status))
        {
            printf("%d exited normally\n", pid);
        }
        else
        {
            printf("%d stopped due to %d\n", pid, status);
            break;
        }
    }



    return 0;
}
