#include <sys/syscall.h>
#include <stdio.h>

#include <libdune/dune.h>
#include "boxer.h"

static int syscall_monitor(struct dune_tf *tf)
{
    switch (tf->rax) {
    case SYS_open:
        printf("opening file %s\n", (char*) ARG0(tf));
        break;
    }

    return 1;
}

void handle_gpfault(struct dune_tf *tf)
{
    const char *code = (char *)tf->rip;

    if (code[0] == 0x0f && code[1] == 0x31)
    {
        unsigned a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        tf->rip += 2;
        tf->rax = a;
        tf->rdx = d;
        return;
    }

    dune_dump_trap_frame(tf);
    dune_die();
}

int main(int argc, char *argv[])
{
    boxer_register_syscall_monitor(syscall_monitor);
    dune_register_intr_handler(13, handle_gpfault);
    return boxer_main(argc, argv);
}
