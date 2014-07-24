#include "libdune/dune.h"
#include <stdio.h>
#include <errno.h>
#include <sys/syscall.h>
#include <stdlib.h>

#define ANSI_COLOR_CYAN    "\x1b[36;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifdef __DEBUG
#define PRINT_ARGS(TF) print_args(TF)
#else 
#define PRINT_ARGS(TF)
#endif

static void color() { printf(ANSI_COLOR_CYAN); }
static void uncolor() { printf(ANSI_COLOR_RESET); }

static void print_args(struct dune_tf *tf) {
	dune_printf("syscall: caught syscall, num=%ld",
		tf->rax);
	dune_printf("\n"
		"\tARG0 = %lx\n"
		"\tARG1 = %lx\n"
		"\tARG2 = %lx\n"
		"\tARG3 = %lx\n"
		"\tARG4 = %lx\n"
		"\tARG5 = %lx\n\n",
		ARG0(tf), ARG1(tf), ARG2(tf),
		ARG3(tf), ARG4(tf), ARG5(tf));
}

void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
	ptent_t *pte;

	//dune_printf("syscall: caught page fault, addr=%p\n", addr);
	dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
	*pte |= PTE_P | PTE_W | PTE_U | PTE_A | PTE_D;
}

void syscall_handler(struct dune_tf *tf)
{
    switch (tf->rax) {
		case SYS_arch_prctl:
		case SYS_brk:
		case SYS_mmap:
		case SYS_mprotect:
		case SYS_munmap:
		case SYS_mremap:
		case SYS_shmat:
		case SYS_clone:
		case SYS_rt_sigaction:
		case SYS_rt_sigprocmask:
		case SYS_sigaltstack:
		case SYS_signalfd:
		case SYS_signalfd4:
		case SYS_rt_sigpending:
		case SYS_rt_sigreturn:
		case SYS_rt_sigsuspend:
		case SYS_rt_sigqueueinfo:
		case SYS_rt_sigtimedwait:
		case SYS_exit_group:
		case SYS_exit:
		case SYS_execve:
		case SYS_gettid:
		case SYS_write:
			PRINT_ARGS(tf);
			dune_passthrough_syscall(tf);
			break;

		default:
			dune_printf("syscall: caught syscall, num=%ld\n",
				tf->rax);
			dune_passthrough_syscall(tf);
			break;
    }
}

void dune_startup() { 
	int ret;

	ret = dune_init_and_enter();
	if(ret) {
		printf("Dune failed to initialize!\n");
		exit(ret);
	}

	color();
	dune_printf("syscall: now printing from dune mode\n");
	uncolor();

	dune_register_pgflt_handler(pgflt_handler);
	dune_register_syscall_handler(syscall_handler);

	dune_printf("syscall: about to call user code\n");
}

int dune_call_user(void *func)
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

