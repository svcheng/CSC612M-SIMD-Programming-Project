section .rodata align=16
    mask_table:
        dd 0x00000000, 0x00000000, 0x00000000, 0x00000000  ; 0 lanes
        dd 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000  ; 1 lane
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000  ; 2 lanes
        dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000  ; 3 lanes

section .text
bits 64
default rel
global vecaddmulxmm

vecaddmulxmm:
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
    cmp r13, 4
    jl bound_check
    vmovdqu xmm1, [r8+rsi]
    vmovdqu xmm2, [r9+rsi]
    vmovdqu xmm3, [rbx+rsi]
    vmulps xmm4, xmm2, xmm3
    vaddps xmm4, xmm1, xmm4

    vmovdqu [rdx+rsi], xmm4

    add rsi, 16
    sub r13, 4
    jmp L1

bound_check:
    test r13, r13
    jz done

    lea rax, [mask_table]
    mov r14, 16
    imul r13, r14
    vmovdqu xmm0, [rax+r13]

    vmaskmovps xmm1, xmm0, [r8  + rsi]   ; B_tail
    vmaskmovps xmm2, xmm0, [r9  + rsi]   ; C_tail
    vmaskmovps xmm3, xmm0, [rbx + rsi]   ; D_tail

    vmulps     xmm4, xmm2, xmm3          ; C * D
    vaddps     xmm4, xmm1, xmm4          ; B + C*D

    ; Masked store: only the valid lanes are written
    vmaskmovps [rdx + rsi], xmm0, xmm4   ; A_tail = result

done:
    pop rbp
    pop rsi
    pop rbx
    pop r14
    pop r13

	ret