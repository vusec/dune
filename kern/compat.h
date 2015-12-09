#ifndef __DUNE_COMPAT_H_
#define __DUNE_COMPAT_H_

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#include <asm/fpu-internal.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#define store_gdt native_store_gdt
#endif

#ifndef X86_CR4_RDWRGSFS
#define X86_CR4_RDWRGSFS X86_CR4_FSGSBASE
#endif

#if !defined(VMX_EPT_AD_BIT)
#define VMX_EPT_AD_BIT          (1ull << 21)
#define VMX_EPT_AD_ENABLE_BIT   (1ull << 6)
#endif

#ifndef VMX_EPT_EXTENT_INDIVIDUAL_BIT
#define VMX_EPT_EXTENT_INDIVIDUAL_BIT           (1ull << 24)
#endif

#ifndef X86_CR4_PCIDE
#define X86_CR4_PCIDE       0x00020000 /* enable PCID support */
#endif

#ifndef SECONDARY_EXEC_ENABLE_INVPCID
#define SECONDARY_EXEC_ENABLE_INVPCID   0x00001000
#endif

#ifndef X86_CR4_FSGSBASE
#define X86_CR4_FSGSBASE    X86_CR4_RDWRGSFS
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
typedef long (*do_fork_hack) (unsigned long, unsigned long, unsigned long,
                              int __user *, int __user *);
static do_fork_hack __dune_do_fork = (do_fork_hack) DO_FORK;
static inline long
dune_do_fork(unsigned long clone_flags, unsigned long stack_start,
         struct pt_regs *regs, unsigned long stack_size,
         int __user *parent_tidptr, int __user *child_tidptr)
{
    struct pt_regs tmp;
    struct pt_regs *me = current_pt_regs();
    long ret;

    memcpy(&tmp, me, sizeof(struct pt_regs));
    memcpy(me, regs, sizeof(struct pt_regs));

    ret = __dune_do_fork(clone_flags, stack_start, stack_size,
                 parent_tidptr, child_tidptr);

    memcpy(me, &tmp, sizeof(struct pt_regs));
    return ret;

}
#else
typedef long (*do_fork_hack) (unsigned long, unsigned long,
                              struct pt_regs *, unsigned long,
                              int __user *, int __user *);
static do_fork_hack dune_do_fork = (do_fork_hack) DO_FORK;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
typedef void (*task_work_run_t) (void);
static task_work_run_t dune_task_work_run = (task_work_run_t)TASK_WORK_RUN;
#else
static void dune_task_work_run(void) { }
#endif

#endif /* __DUNE_COMPAT_H_ */
