#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <libdune/dune.h>
#include "boxer.h"

struct mv_syscall_execution
{
    uint64_t num;
    uint64_t arg[6];
    char arg_buf[128]; /* TODO either mmap on the fly or use other procs mem */
};
struct mv_shared_administration
{
    int num_processes;

    pthread_mutex_t lock;
    pthread_barrier_t barrier;
    int count;

    struct mv_syscall_execution *last_syscall;
    int stop_execution;
};

static struct mv_shared_administration *admin; /* mmap'ed as shared */
static int mv_id = -1; /* local id from 0 to number of procs */

static int syscall_monitor(struct dune_tf *tf)
{
    /*
    fprintf(stderr, "MV%d: syscall %lu (%lx %lx %lx %lx %lx %lx)\n", mv_id,
            tf->rax, ARG0(tf), ARG1(tf), ARG2(tf), ARG3(tf), ARG4(tf),
                     ARG5(tf));
    */
#ifdef INTERCEPT
    if (tf->rax == SYS_getpid)
    {
        tf->rax = -EPERM;
        return 0;
    }
#endif
    return 1;
#if 0
    /* Simple test to check if SYS_open calls match (all others ignored) */
    if (tf->rax != SYS_open)
        return 1;

    pthread_mutex_lock(&admin->lock);
    admin->last_syscall[mv_id].num = tf->rax;
    admin->last_syscall[mv_id].arg[0] = ARG0(tf);

    /* SYS_open has 1 arg - a string, which we need to copy to local buf. */
    /* TODO: can't we look at proc's local mem or so? it should still be there,
     * since the addr space is not shared and the process is still frozen. */
    strncpy(admin->last_syscall[mv_id].arg_buf, (char*)ARG0(tf), 128);
    admin->last_syscall[mv_id].arg_buf[strlen((char*)ARG0(tf))] = '\0';

    admin->count++;

    if (admin->count == admin->num_processes)
    {
        int i;
        for (i = 1; i < admin->num_processes; i++)
        {
            if (strcmp((char*)admin->last_syscall[0].arg_buf,
                       (char*)admin->last_syscall[i].arg_buf))
                fprintf(stderr, "ARG0 mismatch: %d: %s, %d: %s\n",
                        0, (char*)admin->last_syscall[0].arg_buf,
                        i, (char*)admin->last_syscall[i].arg_buf);
        }
        admin->count = 0;
    }
#else
    /* See if order of syscalls is the same - arguments ignored */
    pthread_mutex_lock(&admin->lock);
    admin->last_syscall[mv_id].num = tf->rax;

    admin->count++;

    if (admin->count == admin->num_processes)
    {
        int i;

        admin->stop_execution = 0;

        for (i = 1; i < admin->num_processes; i++)
        {
            if (admin->last_syscall[0].num != admin->last_syscall[i].num)
            {
                fprintf(stderr, "syscall MISMATCH: %d: %lu, \t%d: %lu\n",
                        0, admin->last_syscall[0].num,
                        i, admin->last_syscall[i].num);

                admin->stop_execution = 1;
            }
            /*
            else
                fprintf(stderr, "syscall    match: %d: %lu, \t%d: %lu\n",
                        0, admin->last_syscall[0].num,
                        i, admin->last_syscall[i].num);
            */

        }
        admin->count = 0;
    }
#endif

    pthread_mutex_unlock(&admin->lock);

    pthread_barrier_wait(&admin->barrier);

    /* For now, if some syscalls didn't match, all of them will return an error
     * (and continue execution afterwards...) */
    if (admin->stop_execution)
    {
        tf->rax = -EPERM;
        return 0;
    }

	return 1;
}

/* Sets up administration shared by all processes and start them. */
int main(int argc, char *argv[])
{
    const int nproc = argc - 1;
    int i, tmp;

    if (nproc == 0)
    {
        printf("Usage: %s EXECUTABLE...\n", argv[0]);
        printf("Starts all executables (absolute paths!) in parallel and checks"
               "their syscalls.\n");
        return 1;
    }

    /* Set up administration shared by all processes. */
    admin = mmap(NULL, sizeof(struct mv_shared_administration),
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    admin->last_syscall = mmap(NULL,
            nproc * sizeof(struct mv_syscall_execution), PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    admin->num_processes = nproc;
    admin->count = 0;
    admin->stop_execution = 0;

    /* Initialize synchronization primitives for shared administration. */
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&admin->lock, &m_attr);
    pthread_barrierattr_t b_attr;
    pthread_barrierattr_init(&b_attr);
    pthread_barrierattr_setpshared(&b_attr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&admin->barrier, &b_attr, nproc);

    /* Fork and start all processes as children (via dune sandbox). */
    for (i = 0; i < nproc; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
            perror("fork");
        else if (pid == 0)
        {
            mv_id = i;
            /*
             * Construct new argv to execute in child proc. The loader present
             * in boxer is really limited, and therefore we make that loader
             * load the default system loader, which in turn can load the actual
             * binary.
             */
            char *new_argv[4] = {argv[0], "/lib64/ld-linux-x86-64.so.2",
                                 argv[i + 1], NULL};
            boxer_register_syscall_monitor(syscall_monitor);
            return boxer_main(3, new_argv);
        }
    }

    /* Wait for all child processes to terminate. Not strictly necessary but
       allows them to be killed by SIGINT for instance. */
    for (i = 0; i < nproc; i++)
        wait(&tmp);

    return 0;

}
