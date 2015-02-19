// TODO: make Makefile extract these out of System.map
#define OLD_RSP 0xb0c0
#define KERNEL_STACK 0xb848

.section .text

.global syscall_new

.macro SAVE
    swapgs
    mov %rsp,%gs:OLD_RSP
    mov %gs:KERNEL_STACK,%rsp
    //sti

    sub $0x50,%rsp
    mov %rdi,0x40(%rsp)
    mov %rsi,0x38(%rsp)
    mov %rdx,0x30(%rsp)
    mov %rax,0x20(%rsp)
    mov %r8,0x18(%rsp)
    mov %r9,0x10(%rsp)
    mov %r10,0x8(%rsp)
    mov %r11,(%rsp)
    mov %rax,0x48(%rsp) /* ORIG_RAX */
    mov %rcx,0x50(%rsp)
.endm

.macro CONTINUE
    //cli
    mov 0x50(%rsp),%rcx
    mov (%rsp),%r11
    mov 0x8(%rsp),%r10
    mov 0x10(%rsp),%r9
    mov 0x18(%rsp),%r8
    mov 0x20(%rsp),%rax
    mov 0x30(%rsp),%rdx
    mov 0x38(%rsp),%rsi
    mov 0x40(%rsp),%rdi
    mov %gs:OLD_RSP,%rsp
    //jmp *after_swapgs
.endm

syscall_new:
    SAVE
    // input: rax=nr, args in rdi, rsi, rdx, r10, r8, r9
    // c inp:                 rdi, rsi, rdx, rcx, r8, r9
    // A bit awkward, we want to pass the value of rax too, but don't waste
    // performance on using the stack preferably. For now, overwrite the (barely
    // used) arg5 in r9. TODO: rework this anyway
    // TODO: pass the entire thing above as a struct
    sub $0x10, %rsp
    mov %r9, 0x0(%rsp)
    mov %r10, %rcx
    mov %rax, %r9

    // Pass a pointer into the saved regs 'struct' above to allow changing rax
    lea 0x30(%rsp), %rax
    mov %rax,0x8(%rsp)

    call handle_syscall

    add $0x10, %rsp

    test %rax, %rax
    jnz deny_syscall

    // TODO: this will pop all registers from the stack and then
    // system_call_after_swapgs will again push them... better jump straight
    // into system_call_fastpath, we just have to do the tracesys thing
    // ourselves
    CONTINUE
    jmp *syscall_entry_allow

deny_syscall:
    // We will jump to ret_from_sys_call (stored in syscall_entry_deny), which
    // is right after the normal kernel code does the call into the
    // sys_call_table. handle_syscall should have updated the RAX field already
    // to give the return value, and the SAVE macro imitates the saving behavior
    // of system_call_after_swapgs, so our state should be okay.
    jmp *syscall_entry_deny




