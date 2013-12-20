.globl __signal_call_marshal
	.type	__signal_call_marshal, @function
__signal_call_marshal:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movl    16(%ebp), %esi
	movl	20(%ebp), %ebx
	movl	24(%ebp), %edx
	subl    28(%ebp), %esp
	leal	 4(%esp), %edi
.L0:
	cmpl    $0, %edx
	je      .L5
	movl     (%ebx), %eax
	cmpl    $1,  %eax
	je      .L3
	movl    4(%ebx), %ecx
	addl    $3, %ecx
	shrl	$1, %ecx
	shrl	$1, %ecx
.L2:
	movl    (%esi), %eax
	movl    %eax, (%edi)
	addl	$4, %esi
	addl	$4, %edi
	loop	.L2
	jmp     .L4
.L3:
	fldl	(%esi)
	movl	4(%ebx), %ecx
	cmpl	$8, %ecx
	je		.DF
	fstps	(%edi)
	addl	$4, %edi
	jmp		.DD
.DF:
	fstpl	(%edi)
	addl	$8, %edi
.DD:
	addl	$8, %esi
.L4:
	addl    $8, %ebx
	decl    %edx
	jne     .L0
.L5:
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	movl	12(%ebp), %eax
	call	*%eax
	leave
	ret
	.size	__signal_call_marshal, .-__signal_call_marshal

.globl __signal_call_marshal_1
	.type	__signal_call_marshal_1, @function
__signal_call_marshal_1:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp
	movl    20(%ebp), %esi
	movl	24(%ebp), %ebx
	movl	28(%ebp), %edx
	subl    32(%ebp), %esp
	leal	 8(%esp), %edi
.P0:
	cmpl    $0, %edx
	je      .P5
	movl     (%ebx), %eax
	cmpl    $1,  %eax
	je      .P3
	movl    4(%ebx), %ecx
	addl    $3, %ecx
	shrl	$1, %ecx
	shrl	$1, %ecx
.P2:
	movl    (%esi), %eax
	movl    %eax, (%edi)
	addl	$4, %esi
	addl	$4, %edi
	loop	.P2
	jmp     .P4
.P3:
	fldl	(%esi)
	movl	4(%ebx), %ecx
	cmpl	$8, %ecx
	je		.DF1
	fstps	(%edi)
	addl	$4, %edi
	jmp		.DD1
.DF1:
	fstpl	(%edi)
	addl	$8, %edi
.DD1:
	addl	$8, %esi
.P4:
	addl    $8, %ebx
	decl    %edx
	jne     .P0
.P5:
	movl	16(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	movl	12(%ebp), %eax
	call	*%eax
	leave
	ret
	.size	__signal_call_marshal_1, .-__signal_call_marshal_1
