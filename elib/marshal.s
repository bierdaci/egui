.globl __signal_call_marshal
	.type	__signal_call_marshal, @function
__signal_call_marshal:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	subl	20(%ebp), %esp

	movl	20(%ebp), %ecx
	shrl	$1, %ecx
	shrl	$1, %ecx
	je		.L2
	movl	16(%ebp), %esi
	leal	4(%esp), %edi
.L1:
	movl    (%esi), %eax
	movl    %eax, (%edi)
	addl	$4, %esi
	addl	$4, %edi
	loop	.L1
.L2:
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
	subl	24(%ebp), %esp

	movl	24(%ebp), %ecx
	shrl	$1, %ecx
	shrl	$1, %ecx
	je		.L4
	movl	20(%ebp), %esi
	leal	8(%esp), %edi
.L3:
	movl    (%esi), %eax
	movl    %eax, (%edi)
	addl	$4, %esi
	addl	$4, %edi
	loop	.L3
.L4:
	movl	16(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	movl	12(%ebp), %eax
	call	*%eax
	leave
	ret
	.size	__signal_call_marshal_1, .-__signal_call_marshal_1
