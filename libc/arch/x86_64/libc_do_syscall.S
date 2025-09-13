.global libc_do_syscall
.type libc_do_syscall, @function
.align 4
libc_do_syscall:
    /* Move syscall number in rax so that args form a cdecl function call */
    mov %rdi, %rax           /* Syscall number */
    mov %rsi, %rdi           /* arg1 */
    mov %rdx, %rsi           /* arg2 */
    mov %rcx, %rdx           /* arg3 */
    mov %r8, %r10            /* arg4 stored in r10 because rcx will store rip after syscall */
    mov %r9, %r8             /* arg5 */
    mov 8(%rsp), %r9         /* arg6 */

    /* no need to setup msr, it will be setup at kernel initialization */

    syscall

    /* syscall will put result in rax */
    retq

