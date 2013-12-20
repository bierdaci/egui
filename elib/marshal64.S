#define ARG_OBJ  -64 (%rbp)
#define ARG_FUN  -72 (%rbp)
#define ARG_AP   -80 (%rbp)
#define ARG_NUM  -88 (%rbp)
#define CP_SRC   -96 (%rbp)
#define CP_DST   -104(%rbp)
#define ARG_SIZE -112(%rbp)
#define FLOAT_SW -120(%rbp)
#define ARG_NODE %r10
#define RIGE_I   %r11d
#define FLOT_I   %r12d

.macro _MOV_R v,r1,r2,s,l
	cmpl $4, \s
	jne \l
	movl (\v), \r1
	jmp .MOV_RIGE_END
\l:
	movq (\v), \r2
	jmp .MOV_RIGE_END
.endm

.macro MOV_RIGE v,s
.RIGE_AGAIN:
	leaq .RDI(%rip), %rax
	movl RIGE_I, %ebx
	shll $4,     %ebx
	addq %rbx,   %rax
	jmp *%rax
.RDI:
	_MOV_R \v, %esi, %rsi, \s, .LR1
	_MOV_R \v, %edx, %rdx, \s, .LR2
	_MOV_R \v, %ecx, %rcx, \s, .LR3
	_MOV_R \v, %r8d, %r8,  \s, .LR4
	_MOV_R \v, %r9d, %r9,  \s, .LR5
.MOV_RIGE_END:
	incl RIGE_I
	addq $8, \v
	shrl $4, \s
	jg .RIGE_AGAIN
.endm

.macro _MOV_F v,s,xmm,r
	movsd (\v), \xmm
	cmpl $4, \s
	je \r
	leaq .MOV_FLOT_END(%rip), %rax
	jmp *%rax
\r:
	unpcklpd    \xmm, \xmm
	cvtpd2ps    \xmm, \xmm
	leaq .MOV_FLOT_END(%rip), %rax
	jmp *%rax
.endm

.macro MOV_FLOAT v,s
	leaq .MMU(%rip), %rax
	movl  FLOT_I, %ebx
	imull $37,    %ebx
	addq  %rbx,   %rax
	jmp  *%rax
.MMU:
	_MOV_F \v, \s, %xmm0,.LM0
	_MOV_F \v, \s, %xmm1,.LM1
	_MOV_F \v, \s, %xmm2,.LM2
	_MOV_F \v, \s, %xmm3,.LM3
	_MOV_F \v, \s, %xmm4,.LM4
	_MOV_F \v, \s, %xmm5,.LM5
	_MOV_F \v, \s, %xmm6,.LM6
	_MOV_F \v, \s, %xmm7,.LM7
.MOV_FLOT_END:
.endm

.globl __signal_call_marshal
	.type	__signal_call_marshal, @function
__signal_call_marshal:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	leal	64(%r9d), %eax
	subq	%rax,  %rsp
	andq	$0xfffffffffffffff0, %rsp
	movq	%rdi, ARG_OBJ
	movq	%rsi, ARG_FUN
	movq	%rdx, ARG_AP
	movq	%rcx, ARG_NODE
	movl	%r8d, ARG_NUM
	movl	%r9d, ARG_SIZE
	movl	$0,   RIGE_I
	movl	$0,   FLOT_I
	movq	$0,   CP_SRC
	movq	$0,   CP_DST
 
.LOOP_START:
	cmpl	$0,   ARG_NUM
	je		.CALL_FUNC

	movl	(ARG_NODE),  %ebx
	cmpl	$1,          %ebx
	je  	.FLOAT_S
	movl	$16,         %ebx
	cmpl	%ebx,        4(ARG_NODE)
	jg		.STACK_S

	leal	(,RIGE_I,8), %ebx
	addl	4(ARG_NODE), %ebx
	cmpl	$40,         %ebx
	jg		.STACK_S

	movq	ARG_AP,      %rax
	cmpl	$48,         (%rax)
	je      .STACK_GET

	movq	16(%rax),    %r15
	movl	(%rax),      %ebx
	addq	 %rbx,       %r15
	movl	4(ARG_NODE), %r14d
	MOV_RIGE  %r15,      %r14d
	movq	ARG_AP,      %rax
	movl	4(ARG_NODE), %ebx
	addl	$4,          %ebx
	andl	$0xfffffff8, %ebx
	addl	%ebx,       (%rax)
	jmp	   .LOOP_END

.STACK_GET:
	incl	RIGE_I
	addq	$8,          CP_SRC
	movq	8(%rax),     %r15
	movl	4(ARG_NODE), %r14d
	cmpl	$8,          %r14d
	je		.MOVR9
	movl	(%r15),      %r9d
	jmp    .LOOP_END
.MOVR9:
	movq	(%r15),      %r9
	jmp	   .LOOP_END

.FLOAT_S:
	cmpl	$7,			FLOT_I
	jg		.STACK_S

	movq	ARG_AP,      %rax
	movq	16(%rax),    %r15
	movl	4(%rax),     %ebx
	addq	 %rbx,       %r15
	movl	4(ARG_NODE), %r14d
	MOV_FLOAT %r15,      %r14d
	incl	FLOT_I
	movq	ARG_AP,      %rax
	addl	$16,       4(%rax)
	jmp    .LOOP_END

.STACK_S:
	movq	ARG_AP,      %rax
	movq	8(%rax),     %r14
	addq	CP_SRC,      %r14
	movq	%rsp,        %r15
	addq	CP_DST,      %r15
	movl	$0,          %ebx
	cmpl	$0,     (ARG_NODE)
	je  .COPY_LOOP
	cmpl	$4,    4(ARG_NODE)
	jne .COPY_LOOP

	movsd	 %xmm0,  FLOAT_SW
	movsd	(%r14), %xmm0
	unpcklpd %xmm0, %xmm0
	cvtpd2ps %xmm0, %xmm0
	movsd	 %xmm0, (%r15)
	movsd FLOAT_SW, %xmm0
	addl	$8,    CP_SRC
	addl	$8,    CP_DST
	jmp     .LOOP_END

.COPY_LOOP:
	movq	(%r14, %rbx), %rax
	movq	%rax,       (%r15, %rbx)
	addl	$8,         %ebx
	cmpl	%ebx,      4(ARG_NODE)
	jg      .COPY_LOOP
	addl	%ebx,       CP_SRC
	addl	%ebx,    	CP_DST

.LOOP_END:
	addq	$8, ARG_NODE
	decl	ARG_NUM
	jne    .LOOP_START

.CALL_FUNC:
	movq	ARG_OBJ, %rdi
	movq	ARG_FUN, %rax
	call	*%rax
	movq	%rbp, %rbx
	subq	%rsp, %rbx
	subq	$56,  %rbx
	addq	%rbx, %rsp
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%rbx
	leave
	ret
	.size	__signal_call_marshal, .-__signal_call_marshal
