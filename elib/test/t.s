	.file	"t.c"
	.text
.globl test1
	.type	test1, @function
test1:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp
	movl	12(%ebp), %eax
	movl	%eax, -8(%ebp)
	movl	16(%ebp), %eax
	movl	%eax, -4(%ebp)
	leave
	ret
	.size	test1, .-test1
.globl test
	.type	test, @function
test:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	leal	12(%ebp), %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leal	8(%eax), %edx
	movl	%edx, -4(%ebp)
	fldl	(%eax)
	fstpl	-16(%ebp)
		.value	0x0b0f
	.size	test, .-test
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-8, %esp
	subl	$24, %esp
	fldl	.LC1
	fstpl	12(%esp)
	fldl	.LC2
	fstpl	4(%esp)
	movl	$0x3f800000, %eax
	movl	%eax, (%esp)
	call	test
	leave
	ret
	.size	main, .-main
	.section	.rodata
	.align 8
.LC1:
	.long	0
	.long	1074266112
	.align 8
.LC2:
	.long	0
	.long	1073741824
	.ident	"GCC: (Ubuntu 4.4.3-4ubuntu5.1) 4.4.3"
	.section	.note.GNU-stack,"",@progbits
