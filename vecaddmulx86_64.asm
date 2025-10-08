section .text
bits 64
default rel
global vecaddmulx86_64
vecaddmulx86_64:
    ; rcx - n
    ; rdx - A
    ; r8 - B
    ; r9 - C 
    ; get 5th argument and put in r10 

    push rbx
    push rsi
    push rbp
    mov rbp, rsp
    add rbp, 16
    add rbp, 16

    mov rbx, [rbp+32]
    
    mov rsi, 0 ; index
L1:
    movss xmm0, [r8+rsi*4] ; B[i]
    movss xmm1, [r9+rsi*4] ; C[i]
    movss xmm2, [rbx+rsi*4] ; D[i]
    mulss xmm1, xmm2 ; xmm1 = C[i]*D[i]
    addss xmm0, xmm1 ; xmm0 = xmm0 + xmm1
    movss [rdx+rsi*4], xmm0 ; A[i] = xmm0
    inc rsi
    cmp rcx, rsi
    jne L1

    pop rbp
    pop rsi
    pop rbx

    ret