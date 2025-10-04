
section .data
msg db "Hello World",0

section .text
bits 64
default rel

global asmHello
extern printf

asmHello:
	sub rsp, 8*40
	mov rcx, msg
	call printf
	add rsp, 8*40

	ret