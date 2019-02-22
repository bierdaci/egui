	.text
	.p2align	2
	.global	__signal_call_marshal_1
	.type	__signal_call_marshal_1, %function
__signal_call_marshal_1:
	sub		sp, sp, #16
	stp		X29, X30, [sp]
	mov		X29, sp

	sub		sp, sp, #24
	str		x0, [x29, #-8]
	str		x1, [x29, #-16]
	str		x2, [x29, #-24]
	mov		x19, x3

	//ldr		w11, [x19, 28]
	//ldr		x10, [x19, 16]
	//sxtw	x11, w11
	//add		x11, x10, x11

	mov		w0, 2
	mov		w12, 0

	mov		x10, x4
	mov		w14, w5
	lsl		w15, w14, 3
	sxtw	x15,  w15
	sub		x15, sp, x15
	sub		x15, x15, 8
	add		x15, x15, 15
	bic		x15, x15, 15
	mov		sp, x15
	mov		x15, 0

L0:
	cmp     w14, #0
	beq     L5

	ldr     w13, [x10, #0]
	cmp		w13, 1
	beq		LF

	ldr     w13, [x10, #4]

	ldr		w17, [x19, 24]
	cmp		w17, 0
	beq		L2

	mov		w9,  w17
	add		w17, w17, 8
	str		w17, [x19, 24];

	sxtw	x9,  w9
	ldr		x8,  [x19, 8]
	add		x9,  x8, x9

	cmp		w13, #8
	beq		LR8

LR4:
	cmp		w0, #2
	beq		LR42
	cmp		w0, #3
	beq		LR43
	cmp		w0, #4
	beq		LR44
	cmp		w0, #5
	beq		LR45
	cmp		w0, #6
	beq		LR46
	cmp		w0, #7
	beq		LR47

LR42:
	ldr		w2, [x9]
	b		L1
LR43:
	ldr		w3, [x9]
	b		L1
LR44:
	ldr		w4, [x9]
	b		L1
LR45:
	ldr		w5, [x9]
	b		L1
LR46:
	ldr		w6, [x9]
	b		L1
LR47:
	ldr		w7, [x9]
	b		L1

LR8:
	cmp		w0, #2
	beq		LR82
	cmp		w0, #3
	beq		LR83
	cmp		w0, #4
	beq		LR84
	cmp		w0, #5
	beq		LR85
	cmp		w0, #6
	beq		LR86
	cmp		w0, #7
	beq		LR87

LR82:
	ldr		x2, [x9]
	b		L1
LR83:
	ldr		x3, [x9]
	b		L1
LR84:
	ldr		x4, [x9]
	b		L1
LR85:
	ldr		x5, [x9]
	b		L1
LR86:
	ldr		x6, [x9]
	b		L1
LR87:
	ldr		x7, [x9]

L1:
	add		w0, w0, #1
	b		L4

L2:
	cmp		w13, #8
	beq		L3

	ldr		x17, [x19]
	ldr		w16, [x17]
	str		w16, [sp, x15]
	b		L33

LF:
	add		w12, w12, #1
	cmp		w12, #8
	bgt		L3

	ldr		w17, [x19, 28]
	cmp     w17, 0
	beq     L3
	mov		w9,  w17
	add		w17, w17, 16
	str		w17, [x19, 28];

	sxtw	x9,  w9
	ldr		x8,  [x19, 16]
	add		x9,  x8, x9

	cmp		w12, #1
	beq		LF0
	cmp		w12, #2
	beq		LF1
	cmp		w12, #3
	beq		LF2
	cmp		w12, #4
	beq		LF3
	cmp		w12, #5
	beq		LF4
	cmp		w12, #6
	beq		LF5
	cmp		w12, #7
	beq		LF6
	cmp		w12, #8
	beq		LF7

LF0:
	ldr		d0, [x9]
	b		L4
LF1:
	ldr		d1, [x9]
	b		L4
LF2:
	ldr		d2, [x9]
	b		L4
LF3:
	ldr		d3, [x9]
	b		L4
LF4:
	ldr		d4, [x9]
	b		L4
LF5:
	ldr		d5, [x9]
	b		L4
LF6:
	ldr		d6, [x9]
	b		L4
LF7:
	ldr		d7, [x9]
	b		L4

L3:
	ldr		x17, [x19]
	ldr		x16, [x17]
	str		x16, [sp, x15]
L33:
	add		x15, x15, #8
	add		x17, x17, #8
	str		x17, [x19]

L4:
	add		x10, x10, #8
	sub		w14, w14, #1
	cmp		w14, #0
	bne		L0

L5:
	ldr		x0, [x29, #-8]
	ldr		x9, [x29, #-16]
	ldr		x1, [x29, #-24]
	blr		x9

	mov		sp, x29
	ldp		x29, x30, [sp]
	add		sp, sp, #16
	ret
.Lfunc_end0:
	.size	__signal_call_marshal_1, .Lfunc_end0-__signal_call_marshal_1
