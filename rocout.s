	.text
	.global main
main:
	pushq %rbp
	movq %rsp, %rbp
	movb $5, %dil
	movb $3, %sil
	call _Z3add
	movl $0, %eax
	popq %rbp
	ret
	.global _Z3add
_Z3add:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
	movb %dil, -1(%rbp)
	movb %sil, -2(%rbp)
	movb $48, -3(%rbp)
	movb -3(%rbp), %al
	movb -1(%rbp), %dil
	addb %dil, %al
	movb %al, %al
	movb -2(%rbp), %dil
	addb %dil, %al
	movb %al, -3(%rbp)
	movl $1, %edi
	leaq -3(%rbp), %rax
	movq %rax, %rsi
	movl $1, %edx
	call write
	movzbl -3(%rbp), %eax
	leave
	ret
