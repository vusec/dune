#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "libdune/dune.h"
#include "libdune/cpu-x86.h"

/* Handlers. */
static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
	ptent_t *pte;

	dune_printf("syscall: caught page fault, addr=%p\n", addr);
	dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
	*pte |= PTE_P | PTE_W | PTE_U | PTE_A | PTE_D;
}

static void syscall_handler(struct dune_tf *tf)
{
	assert(tf->rax == SYS_gettid);
	dune_printf("syscall: caught syscall, num=%ld\n", tf->rax);
	dune_passthrough_syscall(tf);
}

/* Utility functions. */
static int dune_call_user(void *func)
{
	int ret;
	unsigned long sp;
	struct dune_tf *tf = malloc(sizeof(struct dune_tf));
	if (!tf)
		return -ENOMEM;

	asm ("movq %%rsp, %0" : "=r" (sp));
	tf->rip = (unsigned long) func;
	tf->rsp = sp - 10000;
	tf->rflags = 0x0;

	ret = dune_jump_to_user(tf);

	return ret;
}

/* User code with system calls. */
void user_main()
{
	int ret = syscall(SYS_gettid);
	assert(ret > 0);
}

int main(int argc, char *argv[])
{
	int ret;

	printf("syscall: not running dune yet\n");

	ret = dune_init_and_enter();
	if (ret) {
		printf("failed to initialize dune\n");
		return ret;
	}

	dune_printf("syscall: now printing from dune mode\n");

	dune_register_pgflt_handler(pgflt_handler);
	dune_register_syscall_handler(syscall_handler);

	dune_printf("syscall: about to call user code\n");
	return dune_call_user(&user_main);
}

