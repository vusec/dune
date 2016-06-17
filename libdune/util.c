/*
 * util.c - this file is for random utilities and hypervisor backdoors
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ucontext.h>

#include "dune.h"

static int dune_puts(const char *buf)
{
    long ret;

    asm volatile("movq $1, %%rax \n\t" // SYS_write
        "movq $1, %%rdi \n\t" // STDOUT
        "movq %1, %%rsi \n\t" // string
        "movq %2, %%rdx \n\t" // string len
        "vmcall \n\t"
        "movq %%rax, %0 \n\t" :
        "=r" (ret) : "r" (buf), "r" (strlen(buf)) :
        "rax", "rdi", "rsi", "rdx");

    return ret;
}

/**
 * dune_printf - a raw low-level printf request that uses a hypercall directly
 *
 * This is intended for working around libc syscall issues.
 */
int dune_printf(const char *fmt, ...)
{
    va_list args;
    char buf[1024];

    va_start(args, fmt);

    vsprintf(buf, fmt, args);

    return dune_puts(buf);
}

void * dune_mmap(void *addr, size_t length, int prot,
         int flags, int fd, off_t offset)
{
    void *ret_addr;

    asm volatile("movq $9, %%rax \n\t" // SYS_mmap
        "movq %1, %%rdi \n\t"
        "movq %2, %%rsi \n\t"
        "movl %3, %%edx \n\t"
        "movq %4, %%r10 \n\t"
        "movq %5, %%r8 \n\t"
        "movq %6, %%r9 \n\t"
        "vmcall \n\t"
        "movq %%rax, %0 \n\t" :
        "=r" ((unsigned long) ret_addr) : "r" ((unsigned long) addr), "r" (length),
        "r" (prot), "r" ((unsigned long) flags), "r" ((unsigned long) fd),
        "r" ((unsigned long) offset) : "rax", "rdi", "rsi", "rdx");

    return ret_addr;
}

/**
 * dune_die - kills the Dune process immediately
 *
 */
void dune_die(void)
{
    asm volatile("movq $60, %rax\n" // exit
             "vmcall\n");
}

/**
 * dune_passthrough_syscall - makes a syscall using the args of a trap frame
 *
 * @tf: the trap frame to apply
 *
 * sets the return code in tf->rax
 */
void dune_passthrough_syscall(struct dune_tf *tf)
{
    asm volatile("movq %2, %%rdi \n\t"
             "movq %3, %%rsi \n\t"
             "movq %4, %%rdx \n\t"
             "movq %5, %%r10 \n\t"
             "movq %6, %%r8 \n\t"
             "movq %7, %%r9 \n\t"
             "vmcall \n\t"
             "movq %%rax, %0 \n\t" :
             "=a" (tf->rax) :
             "a" (tf->rax), "r" (tf->rdi), "r" (tf->rsi),
             "r" (tf->rdx), "r" (tf->rcx), "r" (tf->r8),
             "r" (tf->r9) : "rdi", "rsi", "rdx", "r10",
             "r8", "r9", "memory");
}

sighandler_t dune_signal(int sig, sighandler_t cb)
{
    dune_intr_cb x = (dune_intr_cb) cb; /* XXX */

    if (signal(sig, cb) == SIG_ERR)
        return SIG_ERR;

    dune_register_intr_handler(DUNE_SIGNAL_INTR_BASE + sig, x);

    return NULL;
}

static void *__dune_procmap_print_ptr_val;
static void __dune_procmap_print_ptr(const struct dune_procmap_entry *e)
{
    unsigned long ptr = (unsigned long)__dune_procmap_print_ptr_val;
    if (ptr < e->begin || ptr > e->end)
        return;
    dune_printf("Ptr 0x%lx at offset %lx in %s\n"
            "\t\t(0x%016lx-0x%016lx %c%c%c%c %08lx)\n",
            ptr, ptr - e->begin, e->path,
            e->begin, e->end,
            e->r ? 'R' : '-',
            e->w ? 'W' : '-',
            e->x ? 'X' : '-',
            e->p ? 'P' : 'S',
            e->offset);
}
void dune_print_ptr_mapping(void *ptr)
{
    __dune_procmap_print_ptr_val = ptr;
    dune_procmap_iterate(&__dune_procmap_print_ptr);
}

/* Create an ucontext_t from a given dune_tf (i.e., from userspace state).
 * Useful for e.g. libunwind of userspace. */
void dune_getcontext(ucontext_t *ucp, struct dune_tf *tf)
{
#define R(x) (ucp->uc_mcontext.gregs[x])
    R(0)  = tf->r8;
    R(1)  = tf->r9;
    R(2)  = tf->r10;
    R(3)  = tf->r11;
    R(4)  = tf->r12;
    R(5)  = tf->r13;
    R(6)  = tf->r14;
    R(7)  = tf->r15;
    R(8)  = tf->rdi;
    R(9)  = tf->rsi;
    R(10) = tf->rbp;
    R(11) = tf->rbx;
    R(12) = tf->rdx;
    R(13) = tf->rax;
    R(14) = tf->rcx;
    R(15) = tf->rsp;
    R(16) = tf->rip;
#undef R
}
