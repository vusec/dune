#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MultiVar execution kernel module");

#define num_probes 2
static struct kprobe kp[num_probes];
pid_t tracking_proc;
unsigned long long int last_syscall;


static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    if (regs->ax == 59)
    {
        /* execve */
        char **envp = (char **)regs->dx;
        int i;
        for (i = 0; envp[i] != NULL; i++)
            if (strncmp("MV_RUN_MULTIVAR", envp[i], 15) == 0)
            {
                printk("now tracking %d (%s)\n", current->pid, (char*)regs->di);
                tracking_proc = current->pid;
            }
    }

    if (current->pid == tracking_proc)
    {
        last_syscall = regs->ax;
        //printk("MVKM: pre_handler: %ld %d\n", regs->ax, current->pid);
    }
    return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    if (current->pid == tracking_proc)
        printk("MVKM: handler_post %ld %d\n", regs->ax, current->pid);
}

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    if (current->pid == tracking_proc)
        printk("MVKM: fault_handler %ld %d\n", regs->ax, current->pid);
    return 0;
}


static int __init multivar_init(void)
{
    int i;
    int ret;

    tracking_proc = 0;

    /*
    kp = kmalloc(num_probes * sizeof(struct kprobe), GFP_KERNEL);
    if (kp == NULL)
    {
        printk("MultiVar kernel module: could not allocate mem.\n");
        return 1;
    }
    */
    kp[0].symbol_name = "sys_execve";
    kp[1].symbol_name = "sys_getpid";
    /*
    kp[2].symbol_name = "sys_execve";
    */
    for (i = 0; i < num_probes; i++)
    {
        kp[i].pre_handler = handler_pre;
        /*
        kp[i].post_handler = handler_post;
        kp[i].fault_handler = handler_fault;
        */
        kp[i].post_handler = NULL;
        kp[i].fault_handler = NULL;
        ret = register_kprobe(&kp[i]);
        if (ret < 0)
        {
            printk("MultiVar kernel mod: register_kprobe failed for %s: %d\n",
                    kp[i].symbol_name, ret);
            return ret;
        }
    }
    printk("MultiVar kernel mod started\n");
    return 0;
}

static void __exit multivar_cleanup(void)
{
    int i;
    for (i = 0; i < num_probes; i++)
        unregister_kprobe(&kp[i]);
    printk("MultiVar kernel mod unloaded.\n");
}

module_init(multivar_init);
module_exit(multivar_cleanup);
