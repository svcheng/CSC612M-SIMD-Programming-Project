section .rodata align=32
    mask_table:
        dd 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 0 lanes
        dd 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 1 lane
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 2 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 3 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 4 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000  ; 5 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000  ; 6 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000  ; 7 lanes

section .text
bits 64
default rel
global vecaddmulymm

vecaddmulymm:
	; rcx - n
    ; rdx - A
    ; r8 - B
    ; r9 - C 
    ; get 5th argument and put in rbx 

    push r13
    push r14
    push rbx
    push rsi
    push rbp

    mov rbp, rsp
    add rbp, 16
    add rbp, 32

    mov rbx, [rbp+32]       ; contains the vector D
    
    mov rsi, 0              ; contains the index for traversal

    mov r13, rcx            ; contains the copy of n for boundary checking

L1:
    cmp r13, 8
    jl bound_check
    vmovdqu ymm1, [r8+rsi]
    vmovdqu ymm2, [r9+rsi]
    vmovdqu ymm3, [rbx+rsi]
    vmulps ymm4, ymm2, ymm3
    vaddps ymm4, ymm1, ymm4

    vmovdqu [rdx+rsi], ymm4

    add rsi, 32
    sub r13, 8
    jmp L1

bound_check:
    test r13, r13
    jz done

    lea rax, [mask_table]
    mov r14, 32
    imul r13, r14
    vmovdqu ymm0, [rax+r13]

    vmaskmovps ymm1, ymm0, [r8  + rsi]   ; B_tail
    vmaskmovps ymm2, ymm0, [r9  + rsi]   ; C_tail
    vmaskmovps ymm3, ymm0, [rbx + rsi]   ; D_tail

    vmulps     ymm4, ymm2, ymm3          ; C * D
    vaddps     ymm4, ymm1, ymm4          ; B + C*D

    ; Masked store: only the valid lanes are written
    vmaskmovps [rdx + rsi], ymm0, ymm4   ; A_tail = result

done:
    pop rbp
    pop rsi
    pop rbx
    pop r14
    pop r13

	ret