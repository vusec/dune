#define _GNU_SOURCE /* Needed for RTLD_NEXT */
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "libdune/dune.h"

#include "syscalls.h"


/* Dune handlers. */
static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
	ptent_t *pte;

	dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
	*pte |= PTE_P | PTE_W | PTE_U | PTE_A | PTE_D;
}

static void syscall_handler(struct dune_tf *tf)
{
#ifdef INTERCEPT
    if (tf->rax == SYS_getpid)
    {
        tf->rax = -EPERM;
        return;
    }
#endif

    /*
    if (tf->rax == SYS_getpid)
        dune_printf("SYSCALL: %d %s %d\n", syscall(SYS_getpid),
                syscall_names[tf->rax], tf->rax);
    */

    /* the fork and vfork syscalls are normally not used (e.g. glibc implements
     * fork() with the clone syscall. */
    if (tf->rax == SYS_fork || tf->rax == SYS_vfork)
        assert(0);

    if (tf->rax == SYS_clone)
    {
        /* Child stack ptr (arg 1) 0 means we do a fork-like operation. */
        if (ARG1(tf) == 0)
        {
            /* Since fork (or actually, clone in this case) escapes dune-mode,
             * we save the dune-state (thread-local state) for this thread,
             * fork, reenter dune in the child and restore the state, so we end
             * up with a copy of this proc in dune mode. */
            unsigned long fs = dune_get_user_fs();
            int rc = syscall(SYS_clone, ARG0(tf), ARG1(tf), ARG2(tf), ARG3(tf),
                    ARG4(tf));
            if (rc < 0)
                tf->rax = -errno;
            else
                tf->rax = rc;

            if (rc == 0)
            {
                /* child */
                dune_enter();
                dune_set_user_fs(fs);
            }

            return;

        }
        else
            assert(0); // TODO
    }
    else if (tf->rax == SYS_execve)
    {
        pid_t pid;
        /* Escape dune-mode... only way (we know) how is to fork()...
         * If we exec in dune mode we hit a lot of issues, e.g. EPT failures:
         *  ept: failed to get user page 0
         *  vmx: page fault failure GPA: 0x0, GVA: 0x0
         */
        pid = fork();

        if (pid < 0)
            assert(0);
        if (pid == 0)
        {
            /* In non-dune mode! */
            /* TODO: make sure LD_PRELOAD is propagated so dune mode isn't
             * escaped permanently. */
            execve((char*)ARG0(tf), (char**)ARG1(tf), (char**)ARG2(tf));
            assert(0);
        }
        else
        {
            wait(NULL);
            exit(0);
        }

    }

	dune_passthrough_syscall(tf);
}


/* The type of main. */
typedef int (*main_t)(
        int,
        char **,
        char **
        );

/* The type of __libc_start_main. */
typedef int (*libc_start_main_t)(
        main_t main,
        int argc,
        char **ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void (*stack_end)
        );


/* Original main and it arguments, to be called after transitioning to ring3. */
main_t main_orig;

int __libc_start_main(
        main_t main,
        int argc,
        char ** ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void *stack_end)
        __attribute__ ((noreturn));
int main_predune(int argc, char **argv, char **envp);

/*
 * Override __libc_start_main and replace the main function by our own, which
 * will then set up dune.
 */
int __libc_start_main(
        main_t main,
        int argc,
        char **ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void *stack_end)
{

    libc_start_main_t orig_libc_start_main;

    main_orig = main;

    orig_libc_start_main =
        (libc_start_main_t)dlsym(RTLD_NEXT, "__libc_start_main");
    (*orig_libc_start_main)(&main_predune, argc, ubp_av, init, fini, rtld_fini,
            stack_end);

    exit(EXIT_FAILURE); /* This is never reached. */
}


/*
 * Called by the linker instead of the normal main. Spawns all other processes
 * by forking and sets up dune for all of them.
 */
int main_predune(int argc, char **argv, char **envp)
{
	int ret;
	unsigned long sp;
	struct dune_tf *tf;

	ret = dune_init_and_enter();
	if (ret)
    {
		printf("Dune failed to initialize!\n");
		exit(ret);
	}

	dune_register_pgflt_handler(pgflt_handler);
	dune_register_syscall_handler(syscall_handler);

    /* Switch to ring3 and immidiately call the original main. */
	tf = malloc(sizeof(struct dune_tf));
	if (!tf)
		return -ENOMEM;

	asm ("movq %%rsp, %0" : "=r" (sp));
	tf->rip = (unsigned long)main_orig;
	tf->rsp = sp - 10000;
	tf->rflags = 0x0;
    tf->rdi = argc;
    tf->rsi = (uint64_t)argv;
    tf->rdx = (uint64_t)envp;

	ret = dune_jump_to_user(tf);
    return ret;
}

