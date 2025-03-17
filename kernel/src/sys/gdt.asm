global flush_tss
global jump_user
extern test_user

flush_tss:
    mov ax, 5 * 8
    ltr ax
    ret

; void jump_user(uint64_t addr, uint64_t stack)
jump_user:
    mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax ; SS is handled by iretq

	; set up the stack frame iretq expects
	push (4 * 8) | 3 ; data selector
	push rsi ; stack
	pushf ; rflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push rdi ; instruction address to return to
	iretq