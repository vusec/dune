
.section .text

.global dispatcher

.macro SAVE
    swapgs
    mov %rsp,%gs:0xb0c0
    mov %gs:0xb848,%rsp
    sti

    sub $0x50,%rsp
    mov %rdi,0x40(%rsp)
    mov %rsi,0x38(%rsp)
    mov %rdx,0x30(%rsp)
    mov %rax,0x20(%rsp)
    mov %r8,0x18(%rsp)
    mov %r9,0x10(%rsp)
    mov %r10,0x8(%rsp)
    mov %r11,(%rsp)
    mov %rax,0x48(%rsp)
    mov %rcx,0x50(%rsp)
.endm

.macro CONTINUE
    cli
    mov 0x50(%rsp),%rcx
    mov (%rsp),%r11
    mov 0x8(%rsp),%r10
    mov 0x10(%rsp),%r9
    mov 0x18(%rsp),%r8
    mov 0x20(%rsp),%rax
    mov 0x30(%rsp),%rdx
    mov 0x38(%rsp),%rsi
    mov 0x40(%rsp),%rdi
    mov %gs:0xb0c0,%rsp
    jmp *after_swapgs
.endm

dispatcher:
    // syscall nr is in RAX, let's intercept.


    // -  9 (0x09) sys_mmap
    cmp $0x09, %rax
    je do_mmap

    // - 10 (0x0a) sys_mprotect
    cmp $0x0a, %rax
    je do_mprotect

    // - 13 (0x0d) sys_rt_sigaction
    cmp $0x0d, %rax
    je do_sigaction

    // - 15 (0x0f) sys_rt_sigreturn
    cmp $0x0f, %rax
    je do_sigreturn

    // -  39 (0x27) sys_getpid
    cmp $0x27, %rax
    je do_mv_getpid

    // - 59 (0x3b) sys_execve
    cmp $0x3b, %rax
    je do_execve

    // -  186 (0xba) sys_gettid
    cmp $0xba, %rax
    je do_mv_gettid


    // fallthrough;
    jmp *syscall_handler


do_mmap:
    SAVE
    //call do_c_mmap
    CONTINUE

do_mv_getpid:
    SAVE
    //call do_c_getpid
    CONTINUE

do_mv_gettid:
    SAVE
    //call do_c_gettid
    CONTINUE

do_mprotect:
    SAVE
    //call do_c_mprotect
    CONTINUE

do_sigaction:
    SAVE
    //call do_c_sigaction
    CONTINUE

do_sigreturn:
    SAVE
    //call do_c_sigreturn
    CONTINUE

do_execve:
    SAVE
    //call do_c_execve
    CONTINUE

