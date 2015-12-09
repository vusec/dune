#ifndef __DUNE_COMPAT_H_
#define __DUNE_COMPAT_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#include <asm/i387.h>
#include <asm/fpu-internal.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#define store_gdt native_store_gdt
#endif

#ifndef X86_CR4_RDWRGSFS
#define X86_CR4_RDWRGSFS X86_CR4_FSGSBASE
#endif

#ifndef VMX_EPT_AD_BIT
#define VMX_EPT_AD_BIT          (1ull << 21)
#define VMX_EPT_AD_ENABLE_BIT   (1ull << 6)
#endif

#ifndef VMX_EPT_EXTENT_INDIVIDUAL_BIT
#define VMX_EPT_EXTENT_INDIVIDUAL_BIT           (1ull << 24)
#endif

#ifndef VMX_EPT_1GB_PAGE_BIT
#define VMX_EPT_1GB_PAGE_BIT            (1ull << 17)
#endif

#ifndef VMX_VPID_EXTENT_SINGLE_CONTEXT_BIT
#define VMX_VPID_EXTENT_SINGLE_CONTEXT_BIT      (1ull << 9) /* (41 - 32) */
#endif

#ifndef VMX_VPID_EXTENT_GLOBAL_CONTEXT_BIT
#define VMX_VPID_EXTENT_GLOBAL_CONTEXT_BIT      (1ull << 10) /* (42 - 32) */
#endif

#ifndef SECONDARY_EXEC_RDTSCP
#define SECONDARY_EXEC_RDTSCP            0x00000008
#endif

#ifndef X86_FEATURE_PCID
#define X86_FEATURE_PCID    ( 4*32+17) /* Process Context Identifiers */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#define HOST_IA32_EFER 0x00002c02
#define GUEST_IA32_EFER 0x00002806
#endif

#ifndef __noclone
#define __noclone   __attribute__((__noclone__))
#endif

#ifndef X86_CR4_PCIDE
#define X86_CR4_PCIDE       0x00020000 /* enable PCID support */
#endif

#ifndef SECONDARY_EXEC_ENABLE_INVPCID
#define SECONDARY_EXEC_ENABLE_INVPCID   0x00001000
#endif

#ifndef SECONDARY_EXEC_RDRAND_EXITING
#define SECONDARY_EXEC_RDRAND_EXITING   0x00000800
#endif

#ifndef X86_CR4_FSGSBASE
#define X86_CR4_FSGSBASE    X86_CR4_RDWRGSFS
#endif

/* removed from 3.19 or so */
#ifndef __get_cpu_var
#define __get_cpu_var(var) (*this_cpu_ptr(&(var)))
#endif

/* Not part of vmcs_field enum as of 4.2 */
#define VM_FUNCTION_CONTROLS       0x2018
#define VM_FUNCTION_CONTROLS_HIGH  0x2019
#define EPTP_LIST_ADDR             0x2024
#define EPTP_LIST_ADDR_HIGH        0x2025

#ifdef DO_FORK
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
#endif

#ifdef DO_NOTIFY_RESUME
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)
typedef void (*do_notify_resume_t) (struct pt_regs *, void *, __u32);
static do_notify_resume_t dune_do_notify_resume = (do_notify_resume_t)DO_NOTIFY_RESUME;
void do_notify_resume(struct pt_regs *regs, void *unused, __u32 thread_info_flags)
{
    dune_do_notify_resume(regs, unused, thread_info_flags);
}
#else
void do_notify_resume(struct pt_regs *regs, void *unused, __u32 thread_info_flags)
{
}
#endif
#endif

#endif /* __DUNE_COMPAT_H_ */
