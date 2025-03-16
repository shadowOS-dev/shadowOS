global flush_tss
global jump_user
extern test_user

flush_tss:
    mov ax, 5 * 8
    ltr ax
    ret

; void jump_user(uint64_t addr)
jump_user:
    mov rax, rsp
    push 0x20
    push rax
    pushf
    push 0x18
    push rdi
    iretq