global flush_tss
global jump_user
extern test_user

flush_tss:
    mov ax, 5 * 8
    ltr ax
    ret

; void jump_user(uint64_t addr, uint64_t stack)
jump_user:
    mov ax, 0x23  ; Ring 3 data with bottom 2 bits set for ring 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax   ; SS is handled by iretq

    ; Set up the stack frame iretq expects
    push 0x23     ; Data selector
    push rsi      ; Stack
    pushf         ; Rflags
    push 0x1B     ; Code selector (ring 3 code with bottom 2 bits set for ring 3)
    push rdi      ; Instruction address to return to
    iretq
