#define _GNU_SOURCE /* Needed for RTLD_NEXT */
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "libdune/dune.h"


/* Dune handlers. */
static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
	ptent_t *pte;

	dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
	*pte |= PTE_P | PTE_W | PTE_U | PTE_A | PTE_D;
}

static void syscall_handler(struct dune_tf *tf)
{
    //dune_printf("Syscall %d\n", tf->rax);
#ifdef INTERCEPT
    if (tf->rax == SYS_getpid)
    {
        tf->rax = -EPERM;
        return;
    }
#endif

    if (tf->rax == SYS_clone || tf->rax == SYS_fork)
        dune_printf("SYSCALL: %d\n", tf->rax);
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
    //fprintf(stderr, "MV_PRELOAD: MV_START_DUNE=%s\n", getenv("MV_START_DUNE"));
	int ret;
	unsigned long sp;
	struct dune_tf *tf;
    /*
    if not_forked:
        fork for all childs?
    */

	//printf("mv: not running dune yet\n");

	ret = dune_init_and_enter();
	if (ret)
    {
		printf("Dune failed to initialize!\n");
		exit(ret);
	}

    //dune_printf("now in dune mode\n");

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

