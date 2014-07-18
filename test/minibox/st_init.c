#define _GNU_SOURCE /* Needed for RTLD_NEXT */
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "libdune/dune.h"

/*
 * The type of __libc_start_main.
 */
typedef int (*T_libc_start_main)(
        int *(main) (int, char **, char **),
        int argc,
        char **ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void (*stack_end)
        );

/*
 * The type of main.
 */
typedef int *(*T_main)(
        int,
        char **,
        char **
        );

T_main main_orig;

int __libc_start_main(
        int *(main) (int, char **, char **),
        int argc,
        char ** ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void *stack_end)
        __attribute__ ((noreturn));

int *dune_main(int argc, char **argv, char **envp);
extern void dune_startup();
extern int  dune_call_user(void *func);
extern void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf);
extern void syscall_handler(struct dune_tf *tf);

/********************************
 * Overriding __libc_start_main *
 ********************************/

int __libc_start_main(
        int *(main) (int, char **, char **),
        int argc,
        char **ubp_av,
        void (*init) (void),
        void (*fini) (void),
        void (*rtld_fini) (void),
        void *stack_end)
{

    T_libc_start_main orig_libc_start_main;

    main_orig = main;

    orig_libc_start_main = (T_libc_start_main)dlsym(RTLD_NEXT, "__libc_start_main");
    (*orig_libc_start_main)(&dune_main, argc, ubp_av, init, fini, rtld_fini, stack_end);

    exit(EXIT_FAILURE); /* This is never reached. */
}

int __argc;
char **__argv;
char **__envp;

void dune_main2()
{
    uintptr_t ret = (*main_orig)(__argc, __argv, __envp);
	setenv("LD_PRELOAD", "", 1);
    exit(ret);
}

int *dune_main(int argc, char **argv, char **envp)
{
    dune_startup();

    __argc = argc;
    __argv = argv;
    __envp = envp;
    return dune_call_user(&dune_main2);
}

