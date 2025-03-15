.globl real_handlers
.extern real_handlers

isr_handler_stub:
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %cr0, %rax
    pushq %rax
    movq %cr2, %rax
    pushq %rax
    movq %cr3, %rax
    pushq %rax
    movq %cr4, %rax
    pushq %rax

    movq %ds, %rax
    pushq %rax
    movq %es, %rax
    pushq %rax

    cld

    movq %rsp, %rdi
    movq 168(%rsp), %rbx
    shlq $3, %rbx
    leaq real_handlers(%rip), %rax
    addq %rbx, %rax
    callq *(%rax)

    addq $48, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16, %rsp

    iretq

.macro ISR index
.global _isr\index
.type _isr\index, @function
_isr\index:
.if 0x\index != 8 && 0x\index != 10 && 0x\index != 11 && 0x\index != 12 && 0x\index != 13 && 0x\index != 14 && 0x\index != 17 && 0x\index != 30
    pushq $0
.endif
    pushq $0x\index
    jmp isr_handler_stub
.endm

.macro ISRADDR index
    .quad _isr\index
.endm

.irpc i, 0123456789abcdef
.irpc j, 0123456789abcdef
    ISR \i\j
.endr
.endr

.section .data

.global stubs
.align 8
stubs:
.irpc i, 0123456789abcdef
.irpc j, 0123456789abcdef
    ISRADDR \i\j
.endr
.endr