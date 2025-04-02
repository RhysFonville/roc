	.text
	.global main
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
	movl $-5, -4(%rbp)
	movl $63, -4(%rbp)
	movl -4(%rbp), %eax
	subl $63, %eax
	addl $1, %eax
	movl %eax, %edi
	leaq -4(%rbp), %rax
	movq %rax, %rsi
	movl -4(%rbp), %edx
	addl $7, %edx
	subl $72, %edx
	addl $3, %edx
	movl %edx, %edx
	call write
	movl $0, %eax
	leave
	ret
