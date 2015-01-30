#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>

typedef struct variant_proc
{
    pid_t pid;
    int status;
    struct variant_proc *next;
} variant_proc_t;

typedef struct variant
{
    int syscall_entered;
    unsigned long long int last_syscall;
    int override_return_value;
    unsigned long long int return_value;
    variant_proc_t *procs;
} variant_t;

int procs_halted;

variant_proc_t *find_proc_in_list(variant_proc_t *list, pid_t pid)
{
    variant_proc_t *p = list;
    if (!p)
        return NULL;
    do
    {
        if (p->pid == pid)
            return p;
    } while ((p = p->next));
    return NULL;
}

variant_proc_t *add_proc_to_list(variant_proc_t **list, variant_proc_t *proc)
{
    if (*list == NULL)
        *list = proc;
    else
    {
        variant_proc_t *p = *list;
        while (p->next)
            p = p->next;
        p->next = proc;
    }
    return proc;
}
variant_proc_t *new_proc(pid_t pid)
{
    variant_proc_t *proc = malloc(sizeof(variant_proc_t));
    proc->pid = pid;
    proc->status = 0;
    proc->next = NULL;
    return proc;
}

variant_proc_t *find_proc(variant_t *variants, int num_variants, pid_t pid)
{
    int i;
    variant_proc_t *p;
    for (i = 0; i < num_variants; i++)
        if ((p = find_proc_in_list(variants[i].procs, pid)))
            return p;
    return NULL;
}

variant_t *find_proc_variant(variant_t *variants, int num_variants, pid_t pid)
{
    int i;
    for (i = 0; i < num_variants; i++)
        if (find_proc_in_list(variants[i].procs, pid))
            return &variants[i];
    return NULL;
}

variant_proc_t *pop_proc_from_list(variant_proc_t **list, pid_t pid)
{
    variant_proc_t *p = *list, *prev = *list;
    if (!p)
        return NULL;
    if (p->pid == pid)
    {
        *list = p->next;
        p->next = NULL;
        return p;
    }
    while ((p = p->next))
    {
        if (p->pid == pid)
        {
            prev->next = p->next;
            p->next = NULL;
            return p;
        }
        prev = p;
    }
    return NULL;
}

void print_list(variant_proc_t *list)
{
    variant_proc_t *p = list;
    printf("List: ");
    if (!p)
    {
        printf("<empty>\n");
        return;
    }
    do
    {
        printf("%d, ", p->pid);
    } while ((p = p->next));
    printf("\n");
}

void print_lists(variant_t *variants, int num_variants)
{
    int i;
    for (i = 0; i < num_variants; i++)
    {
        printf("variant %d: ", i);
        print_list(variants[i].procs);
    }
}

void setup(int nvar, char **program, variant_t *variants)
{
    int i;
    pid_t pid;
    int status;
    struct user_regs_struct regs;


    for (i = 0; i < nvar; i++)
    {
        pid = fork();

        if (pid < 0)
            perror("fork");

        if (pid == 0)
        {
            //printf(" # ptrace child running, pid=%d\n", getpid());
            //printf(" # Starting %s\n", program[0]);
            if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
                perror("ptrace");
            execvp(program[0], program);
            perror("execve");
        }
        else
        {
            variant_proc_t *proc;
            variants[i].syscall_entered = 0;
            variants[i].override_return_value = 0;
            variants[i].procs = NULL;
            proc = new_proc(pid);
            add_proc_to_list(&variants[i].procs, proc);

            pid_t pid2 = waitpid(pid, &status, 0);
            assert(pid == pid2);
            assert(WIFSTOPPED(status));
            ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEVFORK);
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            assert(regs.orig_rax == SYS_execve);
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
    }

}

void handle_syscall(variant_t *variants, int nvar, pid_t pid)
{
    struct user_regs_struct regs;
    variant_t *variant;
    int i;
    int mismatch;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    //printf(" # syscall %d %lld\n", pid, regs.orig_rax);
    return;

    variant = find_proc_variant(variants, nvar, pid);
    if (variant->syscall_entered)
    {
        /* On syscall-exit, optionally override return value of syscall and then
         * resume. */
        variant->syscall_entered = 0;

        if (variant->override_return_value)
        {
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            regs.rax = variant->return_value;
            ptrace(PTRACE_SETREGS, pid, NULL, &regs);
            variant->override_return_value = 0;
        }

        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        return;
    }

    /* Barrier on syscall-enter. */

    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    variant->last_syscall = regs.orig_rax;
    variant->syscall_entered = 1;

    procs_halted++;

    if (procs_halted < nvar)
        return;

    procs_halted = 0;

    mismatch = 0;
    for (i = 1; i < nvar; i++)
    {
        if (variants[0].last_syscall != variants[i].last_syscall)
        {
            printf("syscall mismatch:  %d: %llu, %d: %llu\n",
                    i, variants[0].last_syscall,
                    i, variants[i].last_syscall);
            mismatch = 1;
            break;
        }
    }
    for (i = 0; i < nvar; i++)
    {
        if (mismatch)
        {
            /* Replace the syscall by something harmless (getpid) and override
             * return value when syscall is done. */
            variants[i].override_return_value = 1;
            variants[i].return_value = -EPERM;
            //ptrace(PTRACE_POKEUSER, children[i].pid, 8 * ORIG_RAX, SYS_getpid);
        }
        //ptrace(PTRACE_SYSCALL, variants[i].pid, NULL, NULL);
    }
}

int main(int argc, char **argv)
{
    int nvar = 1;
    variant_t *variants;
    variant_proc_t *orphans = NULL;
    int status, event, sig;
    pid_t pid;

    printf(" # ptrace monitor running, pid=%d\n", getpid());

    if (getenv("MV_NUM_PROC"))
        nvar = atoi(getenv("MV_NUM_PROC"));

    procs_halted = 0;
    variants = malloc(sizeof(variant_t) * nvar);
    if (variants == NULL)
        perror("malloc");

    setup(nvar, &argv[1], variants);

    close(fileno(stdout));
    close(fileno(stderr));
    close(fileno(stdin));

    while (1)
    {
        if ((pid = wait(&status)) == -1)
            break;

        event = status >> 16;
        sig = WSTOPSIG(status);

        if (WIFSTOPPED(status) && sig == (SIGTRAP | 0x80))
        {
            handle_syscall(variants, nvar, pid);
        }
        else if (WIFEXITED(status))
        {
            variant_t *var;
            variant_proc_t *proc;
            //printf(" # %d exited normally\n", pid);
            var = find_proc_variant(variants, nvar, pid);
            if (var)
                proc = pop_proc_from_list(&var->procs, pid);
            else
                proc = pop_proc_from_list(&orphans, pid);
            assert(proc);
            free(proc);
            //print_lists(variants, nvar);
        }
        else if (WIFSTOPPED(status) && (sig == SIGCHLD || sig == SIGTERM))
        {
            //printf(" # %d signal\n", pid);
            ptrace(PTRACE_SYSCALL, pid, NULL, WSTOPSIG(status));
        }
        else if (WIFSTOPPED(status) && sig == SIGSTOP)
        {
            //printf(" # %d SIGSTOP\n", pid);
            variant_proc_t *proc = find_proc(variants, nvar, pid);
            if (!proc)
            {
                proc = new_proc(pid);
                add_proc_to_list(&orphans, proc);
            }
            proc->status = SIGSTOP;

            //ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
        else if (event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK || event == PTRACE_EVENT_CLONE)
        {
            pid_t new_pid;
            variant_proc_t *proc;
            variant_t *var;
            ptrace(PTRACE_GETEVENTMSG, pid, 0, &new_pid);
            assert(find_proc(variants, nvar, new_pid) == NULL);
            proc = pop_proc_from_list(&orphans, new_pid);
            if (proc)
                assert(proc->status == SIGSTOP);
            else
            {
                proc = new_proc(new_pid);
                waitpid(new_pid, &status, 0);
                assert(WIFSTOPPED(status));
            }
            var = find_proc_variant(variants, nvar, pid);
            add_proc_to_list(&var->procs, proc);
            proc->status = 0;
            //printf(" # %d forked into %d\n", pid, new_pid);
            //print_lists(variants, nvar);
            ptrace(PTRACE_SETOPTIONS, new_pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACEVFORK);
            ptrace(PTRACE_SYSCALL, new_pid, NULL, NULL);
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        }
        else
        {
            printf(" # %d stopped due to %d\n", pid, status);
            printf(" # %d ifstopped: %d\n", pid, WIFSTOPPED(status));
            printf(" # %d ifsignalled: %d\n", pid, WIFSIGNALED(status));
            printf(" # %d term sig: %d\n", pid, WTERMSIG(status));
            printf(" # %d stopping sig: %s (%d)\n", pid, strsignal(sig), sig);
            break;
        }
    }



    return 0;
}
