/*
 * trap.c - handles system calls, page faults, and other traps
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <asm/prctl.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <utime.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <pthread.h>
#include <err.h>
#include <linux/futex.h>
#include <linux/unistd.h>

#include "sandbox.h"
#include "boxer.h"
#include "libdune/cpu-x86.h"

static boxer_syscall_cb _syscall_monitor;
static pthread_mutex_t _syscall_mtx;

void boxer_register_syscall_monitor(boxer_syscall_cb cb)
{
	_syscall_monitor = cb;
}

static void syscall_do_foreal(struct dune_tf *tf)
{
	switch (tf->rax) {
	case SYS_arch_prctl:
		switch (ARG0(tf)) {
		case ARCH_GET_FS:
			*((unsigned long*) ARG1(tf)) = dune_get_user_fs();
			tf->rax = 0;
			break;

		case ARCH_SET_FS:
			dune_set_user_fs(ARG1(tf));
			tf->rax = 0;
			break;

		default:
			tf->rax = -EINVAL;
			break;
		}
		break;

	case SYS_brk:
		tf->rax = umm_brk((unsigned long) ARG0(tf));
		break;

	case SYS_mmap:
		tf->rax = (unsigned long) umm_mmap((void *) ARG0(tf),
			(size_t) ARG1(tf), (int) ARG2(tf), (int) ARG3(tf),
			(int) ARG4(tf), (off_t) ARG5(tf));
		break;

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
	default:
		dune_passthrough_syscall(tf);
		break;
	}
}

static void syscall_do(struct dune_tf *tf)
{
	int need_lock = 0;

	switch (tf->rax) {
	case SYS_mmap:
	case SYS_mprotect:
	case SYS_munmap:
		need_lock = 1;
		break;
	}

	if (need_lock && pthread_mutex_lock(&_syscall_mtx))
		err(1, "pthread_mutex_lock()");

	syscall_do_foreal(tf);

	if (need_lock && pthread_mutex_unlock(&_syscall_mtx))
		err(1, "pthread_mutex_unlock()");
}

static void syscall_handler(struct dune_tf *tf)
{
	_syscall_monitor(tf);

	syscall_do(tf);
}

int trap_init(void)
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

	if (pthread_mutex_init(&_syscall_mtx, &attr))
		return -1;

	dune_register_syscall_handler(&syscall_handler);

	return 0;
}
