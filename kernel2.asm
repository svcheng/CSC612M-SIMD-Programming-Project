section .text
bits 64
default rel
global kernel2
kernel2:
    ; rcx - n
    ; rdx - A
    ; r8 - B
    ; r9 - C 
    ; get 5th argument and put in r10 
    push rbp
    mov rbp, rsp
    add rbp, 16
    mov r10, [rbp+32]
    pop rbp
    
    mov rax, 0 ; index
L1:
    movss xmm0, [r8+rax*4] ; B[i]
    movss xmm1, [r9+rax*4] ; C[i]
    movss xmm2, [r10+rax*4] ; D[i]
    mulss xmm1, xmm2 ; xmm1 = C[i]*D[i]
    addss xmm0, xmm1 ; xmm0 = xmm0 + xmm1
    movss [rdx+rax*4], xmm0 ; A[i] = xmm0
    inc rax
    cmp rcx, rax
    jnz L1
    ret