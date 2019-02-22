      .text
      .syntax unified
      .eabi_attribute 67, "2.09"  @ Tag_conformance
      .cpu    arm7tdmi
      .eabi_attribute 6, 2    @ Tag_CPU_arch
      .eabi_attribute 8, 1    @ Tag_ARM_ISA_use
      .eabi_attribute 9, 1    @ Tag_THUMB_ISA_use
      .eabi_attribute 34, 0   @ Tag_CPU_unaligned_access
      .eabi_attribute 15, 1   @ Tag_ABI_PCS_RW_data
      .eabi_attribute 16, 1   @ Tag_ABI_PCS_RO_data
      .eabi_attribute 17, 2   @ Tag_ABI_PCS_GOT_use
      .eabi_attribute 20, 1   @ Tag_ABI_FP_denormal
      .eabi_attribute 21, 1   @ Tag_ABI_FP_exceptions
      .eabi_attribute 23, 3   @ Tag_ABI_FP_number_model
      .eabi_attribute 24, 1   @ Tag_ABI_align_needed
      .eabi_attribute 25, 1   @ Tag_ABI_align_preserved
      .eabi_attribute 38, 1   @ Tag_ABI_FP_16bit_format
      .eabi_attribute 18, 4   @ Tag_ABI_PCS_wchar_t
      .eabi_attribute 26, 2   @ Tag_ABI_enum_size
      .eabi_attribute 14, 0   @ Tag_ABI_PCS_R9_use
	.p2align	2
	.global	__signal_call_marshal_1
	.type	__signal_call_marshal_1, %function
__signal_call_marshal_1:
	.fnstart
	push	{r8}
	mov		r8, sp
	push	{r4, r5, r6, r7, r9, r10, fp, lr}
	mov		fp, sp
	sub		sp, sp, #8
	str		r0, [fp, #-4]
	str		r1, [fp, #-8]
	str		r2, [fp, #-12]

	mov   	r4, r3
	ldr		r5, [r8, #4]
	ldr		r6, [r8, #8]
	ldr		r7, [r8, #12]
	sub		sp, sp, r7
	sub		sp, sp, #4
	add		sp, sp, #7
	bic		sp, sp, #7

	mov		r7, sp
	mov		r0, #8

L0:
	cmp     r6, #0
	beq     L5

	ldr     r9, [r5, #4]

	cmp		r0, #15
	bgt		L1
	mov		r10, r9
	sub		r10, r10, #1
	add		r0, r0, r10
	bic		r0, r0, r10
	cmp		r0, #15
	bgt		L1

	mov		r9, r9, LSR#2
LR:
	cmp		r9, #0
	beq		L4
	sub		r9, r9, #1

	cmp		r0, #8
	beq		LR1
	cmp		r0, #12
	beq		LR2
LR1:
	ldr		r2, [r4]
	add		r4, r4, #4
	add		r0, r0, #4
	b		LR
LR2:
	ldr		r3, [r4]
	add		r4, r4, #4
	add		r0, r0, #4
	b		LR

L1:
	cmp		r9, #4
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

L4:
	add		r5, #8
	sub		r6, r6, #1
	cmp		r6, #0
	bne		L0
L5:
	ldr		r0, [fp, #-4]
	ldr		r4, [fp, #-8]
	ldr		r1, [fp, #-12]
	mov		lr, pc
	bx		r4
	mov		sp, fp
	pop		{r4, r5, r6, r7, r9, r10, fp, lr}
	pop		{r8}
	bx		lr
	.fnend
