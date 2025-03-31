	.text
	.global main
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
	movl $48, -4(%rbp)
	movl $5, -8(%rbp)
	movl $3, -12(%rbp)
	movl -4(%rbp), %eax
	movl -8(%rbp), %edi
	addl %edi, %eax
	movl %eax, %eax
	movl -12(%rbp), %edi
	addl %edi, %eax
	movl %eax, -16(%rbp)
	movl $1, %edi
	leaq -16(%rbp), %rax
	movq %rax, %rsi
	movl $1, %edx
	call write
	movl $0, %eax
	leave
	ret
