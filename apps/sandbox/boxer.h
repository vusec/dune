#ifndef __BOXER_H__
#define __BOXER_H__

typedef int (*boxer_syscall_cb)(struct dune_tf *tf);
typedef void (*boxer_syscall_post_cb)(struct dune_tf *tf, uint64_t syscall_no);

extern void boxer_register_syscall_monitor(boxer_syscall_cb cb);
extern void boxer_register_syscall_monitor_post(boxer_syscall_post_cb cb);
extern int boxer_main(int argc, char *argv[]);

#endif /* __BOXER_H__ */
