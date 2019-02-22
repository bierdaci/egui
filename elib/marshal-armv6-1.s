	.arch armv6
	.eabi_attribute 27, 3
	.eabi_attribute 28, 1
	.fpu vfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 6
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.file	"marshal.c"
	.text
	.align	2
	.global	__signal_call_marshal
	.type	__signal_call_marshal, %function
__signal_call_marshal:
	stmfd	sp!, {fp, lr, r4, r5, r6, r7, r8, r9, r10, r12}
	mov		fp, sp
	sub		sp, sp, #8
	str		r0, [fp, #-4]
	str		r1, [fp, #-8]

	mov   	r4, r2
	mov		r5, r3
	ldr		r6, [fp, #40]
	ldr		r7, [fp, #44]
	sub		sp, sp, r7
	sub		sp, sp, #4
	add		sp, sp, #7
	bic		sp, sp, #7
	mov		r7, sp
	mov		r8, #0
	mov		r9, #0
	mov		r0, #4

L0:
	cmp     r6, #0
	beq     L5
	ldr     r10, [r5]
	cmp     r10, #1
	beq     L3

	ldr     r12, [r5, #4]

	cmp		r0, #15
	bgt		L1
	mov		r10, r12
	sub		r10, r10, #1
	add		r0, r0, r10
	bic		r0, r0, r10
	cmp		r0, #15
	bgt		L1

	mov		r12, r12, LSR#2
LR:
	cmp		r12, #0
	beq		L4
	sub		r12, r12, #1

	cmp		r0, #4
	beq		LR1
	cmp		r0, #8
	beq		LR2
	cmp		r0, #12
	beq		LR3
LR1:
	ldr		r1, [r4]
	add		r4, r4, #4
	add		r0, r0, #4
	b		LR
LR2:
	ldr		r2, [r4]
	add		r4, r4, #4
	add		r0, r0, #4
	b		LR
LR3:
	ldr		r3, [r4]
	add		r4, r4, #4
	add		r0, r0, #4
	b		LR

L1:
	cmp		r12, #4
	beq		L2

	add		r4, r4, #7
	bic		r4, r4, #7
	add		r7, r7, #7
	bic		r7, r7, #7

	ldr		r10, [r4]
	str		r10, [r7]
	add		r4, r4, #4
	add		r7, r7, #4

L2:
	ldr		r10, [r4]
	str		r10, [r7]
	add		r4, r4, #4
	add		r7, r7, #4
	b		L4
L3:
	add		r4, r4, #7
	bic		r4, r4, #7
	ldr		r10, [r5, #4]
	cmp		r10, #8
	beq		_DD

_DF:
	mov		r10,  r8, LSR#1
	cmp		r9, r10
	bne		_DF1

	mov		r10,  r8
	add		r8, r8, #1
	mov		r9, r8, LSR#1

	b		ALLOC_DF

_DF1:
	mov		r10,  r8
	mov		r8, r9, LSL#1
	b		ALLOC_DF

_DD:
	mov		r10, r8, LSR#1

	cmp		r9, r10
	bne		_DD2

	tst		r8, #1
	bne		_DD1

	mov		r10,  r9
	add		r9, r9, #1
	mov		r8, r9, LSL#1

	b		ALLOC_DD

_DD1:
	add		r9, r9, #1
	mov		r10,  r9
	add		r9, r9, #1
	b		ALLOC_DD

_DD2:
	mov		r10,  r9
	add		r9, r9, #1
	b		ALLOC_DD

ALLOC_DF:
	cmp		r10, #15
	bgt		_SIZE1
	mov		r12, #12
	mul		r10, r10, r12
	ldr		r12, =JUMP_DF
	add		r10, r10, r12
	blx		r10

ALLOC_DD:
	cmp		r10, #7
	bgt		_SIZE2
	mov		r12, #8
	mul		r10, r10, r12
	ldr		r12, =JUMP_DD
	add		r10, r10, r12
	blx		r10

JUMP_DD:
	fldd	d0, [r4]
	b		_VP
	fldd	d1, [r4]
	b		_VP
	fldd	d2, [r4]
	b		_VP
	fldd	d3, [r4]
	b		_VP
	fldd	d4, [r4]
	b		_VP
	fldd	d5, [r4]
	b		_VP
	fldd	d6, [r4]
	b		_VP
	fldd	d7, [r4]
	b		_VP

JUMP_DF:
	fldd	d8, [r4]
	fcvtsd	s0, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s1, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s2, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s3, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s4, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s5, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s6, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s7, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s8, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s9, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s10, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s11, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s12, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s13, d8
	b		_VP
	fldd	d8, [r4]
	fcvtsd	s14, d8
	b		_VP
	fcvtsd	s15, d8
	b		_VP

_SIZE1:
	fldd	d8, [r4]
	fcvtsd	s16, d8
	fsts	s16, [r7]
	add		r7, r7, #4
	b		_VP

_SIZE2:
	add		r7, r7, #7
	bic		r7, r7, #7
	fldd	d8, [r4]
	fstd	d8, [r7]
	add		r7, r7, #8

_VP:
	add		r4, #8
L4:
	add		r5, #8
	sub		r6, r6, #1
	cmp		r6, #0
	bne		L0
L5:
	ldr		r0, [fp, #-4]
	ldr		r4, [fp, #-8]
	blx		r4
	mov		sp, fp
	ldmfd	sp!, {fp, pc, r4, r5, r6, r7, r8, r9, r10, r12}
	.size	__signal_call_marshal, .-__signal_call_marshal
	.ident	"GCC: (Raspbian 4.9.2-10) 4.9.2"
	.section	.note.GNU-stack,"",%progbits
