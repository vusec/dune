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
#include <asm/percpu.h>

asmlinkage long (*real_chdir)(const char __user *filename);
void (*syscall_handler)(void);
void (*after_swapgs)(void);
long unsigned int orig_reg;

unsigned long *kernel_stack_addr ;
unsigned long *old_rsp_addr;

extern void dispatcher(void);

void do_c_mmap(void) {
    printk("MMAP!\n");
    return;
}
void do_c_mprotect(void) {
    printk("MPROTECT!\n");
    return;
}
void do_c_sigaction(void) {
    printk("SIGACTION!\n");
    return;
}
void do_c_sigreturn(void) {
    printk("SIGRETURN!\n");
    return;
}
void do_c_execve(void) {
    printk("EXECVE!\n");
    return;
}
void do_c_getpid(void) {
    printk("GETPID!\n");
    return;
}
void do_c_gettid(void) {
    printk("GETTID!\n");
    return;
}


int __init mod_init(void){
    unsigned int low = 0, high = 0;
    long unsigned int address;
    int i;
    unsigned long sym_addr = kallsyms_lookup_name("system_call_after_swapgs");

    rdmsr(0xC0000082, low, high);
    printk("Low:%x\tHigh:%x\n", low,high);
    address = 0;
    address |= high;
    address = address << 32;
    address |= low;

    syscall_handler = (void (*)(void)) address;
    after_swapgs = (void (*)(void)) sym_addr;

    printk("Syscall Handler: %lx\n", address);
    printk("&SyscallHandler: %llx\n", (long unsigned long int) &syscall_handler);
    printk("system_call_after_swapgs: %lx\n", sym_addr);

    for (i = 0; i < 256; i++) {
        printk("%02x ", (unsigned char) *(unsigned char *)(address+i));
    }
    printk("\n");

    address = (long unsigned int) dispatcher;
    printk("dispatcher: %lx\n", address);

    for (i = 0; i < 256; i++) {
        printk("%02x ", (unsigned char) *(unsigned char *)(address+i));
    }
    printk("\n");

    wrmsrl(MSR_LSTAR,dispatcher);

    printk("bye\n");
    return 0;
}

void __exit cleanup(void){
    printk("Exit\n");
    wrmsrl(MSR_LSTAR, syscall_handler);
}

module_init(mod_init);
module_exit(cleanup);
MODULE_LICENSE("GPL");
