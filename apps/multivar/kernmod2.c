#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <linux/mman.h>
#include <asm/proto.h>
#include <asm/delay.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/linkage.h>
#include <linux/delay.h>
#include <asm/percpu.h>

MODULE_AUTHOR("Koen Koning");
MODULE_DESCRIPTION("Syscall interposition");
MODULE_LICENSE("GPL");

//#define INTERCEPT

void *syscall_entry_allow;
void *syscall_entry_deny;
uint64_t old_lstar;

pid_t tracking_proc;

extern void syscall_new(void);

int handle_syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3,
        uint64_t arg4, uint64_t syscallnr, uint64_t arg5, uint64_t *rax)
{
    /* First check if we should track this program. */
    if (syscallnr == 59) // execve
    {
        char **envp = (char **)arg2;
        int i;
        //printk("multivar: syscall: %d %llu(%llu, %llu, %llu, %llu, %llu, %llu)\n",
                //current->pid, syscallnr, arg0, arg1, arg2, arg3, arg4, arg5);
        for (i = 0; envp[i] != NULL; i++)
            if (strncmp("MV_RUN_MULTIVAR", envp[i], 15) == 0)
            {
                printk("multivar: now tracking %d (%s)\n", current->pid, (char*)arg0);
                tracking_proc = current->pid;
            }
    }

    /* Exit immediately if we are not interested in this program. */
    if (tracking_proc != current->pid)
        return 0;

    //printk("multivar: syscall: %d %llu(%llu, %llu, %llu, %llu, %llu, %llu)\n",
            //current->pid, syscallnr, arg0, arg1, arg2, arg3, arg4, arg5);

#ifdef INTERCEPT
    switch (syscallnr)
    {
    case 39: /* getpid */
        //printk("multivar: GETPID!!\n");
        *rax = -1;
        return 1; /* block syscall */
    case 60: /* exit */
    case 231: /* exit_group */
        printk("multivar: Tracking proc %d exiting\n", current->pid);
        break;
    case 309: /* getcpu - TEST! */
        //printk("multivar: changing getcpu into getpid.\n");
        *rax = 39;
        break;
    }
#endif

    return 0;
}

void print_bytestream(char *name, void *pointer, int bytes) {
    int i;
    printk("%s @ %p:\n", name, pointer);
    for (i = 0; i < bytes; i++)
        printk("%02x ", (unsigned char) *(unsigned char *)(pointer + i));
    printk("\n");
}

void update_lstar(void *new_lstar) {
    printk("multivar: Setting LSTAR to %p\n", new_lstar);
    wrmsrl(MSR_LSTAR, new_lstar);
}

void read_lstar(void *tmp) {
    uint64_t lstar;
    rdmsrl(MSR_LSTAR, lstar);
    printk("multivar: LSTAR = %p\n", (void*)lstar);
}

int __init mod_init(void){
    printk("multivar: Loading...\n");

    syscall_entry_allow = (void*)kallsyms_lookup_name("system_call_after_swapgs");
    syscall_entry_deny = (void*)kallsyms_lookup_name("ret_from_sys_call");
    rdmsrl(MSR_LSTAR, old_lstar);

    printk("after_swapgs: %p\n", syscall_entry_allow);
    printk("ret_from_sys_call: %p\n", syscall_entry_deny);
    printk("old_lstar: %#llx\n", old_lstar);

    print_bytestream("old_lstar", (void*)old_lstar, 256);

    on_each_cpu(update_lstar, syscall_new, 1);
    on_each_cpu(read_lstar, NULL, 1);

    printk("multivar: Done\n");
#ifdef INTERCEPT
    printk("multivar: Running with interception enabled\n");
#endif

    return 0;
}

void __exit cleanup(void){
    printk("multivar: Exit %#llx\n", old_lstar);
    if (!old_lstar)
        return;
    on_each_cpu(read_lstar, NULL, 1);
    on_each_cpu(update_lstar, (void *)old_lstar, 1);
    old_lstar = 0;
    msleep(1000);
}

module_init(mod_init);
module_exit(cleanup);
