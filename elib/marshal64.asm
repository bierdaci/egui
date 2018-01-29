.CODE
;IN_OBJ		rbp+10h	rcx
;IN_FN		rbp+18h	rdx
;IN_VA		rbp+20h	r8
;IN_AN		rbp+28h	r9
;IN_NUM		rbp+30h
;IN_SIZE	rbp+38h

__signal_call_marshal PROC
	push	rbp
	mov		rbp, rsp
;	sub		rsp, 16

;	mov		qword ptr [rbp-8], rbx
;	mov		qword ptr [rbp-16], rsi
;	mov		qword ptr [rbp-24], rdi

	mov		qword ptr [rbp+10h], rcx
	mov		qword ptr [rbp+18h], rdx
	mov		qword ptr [rbp+28h], r9

	mov		ecx, dword ptr [rbp+30h]
	mov		eax, ecx
	mov		ebx, 8
	mul		ebx
	sub		rsp, rax
	sub		rsp, 8
	and		rsp, 0fffffffffffffff0h
	add		rsp, 8
	mov		rsi, r8
	mov		rdi, rsp
	mov		rdx, r9

L1:
	cmp		ecx, 0
	je      L5
	mov		ebx, dword ptr [rdx]
	cmp     ebx, 1
	je      L3

L2:
	mov		r9, qword ptr [rsi]
	mov		qword ptr [rdi], r9
	jmp		L4

L3:
	movsd	xmm0, mmword ptr [rsi] 
	movsd	mmword ptr [rdi], xmm0

L4:
	add		rsi, 8
	add		rdi, 8
	add		rdx, 8
	dec		ecx
	jne		L1

L5:
	mov		rcx, qword ptr [rbp+10h]

	mov		eax, dword ptr [rbp+30h]
	cmp		eax, 0
	je		L6
	mov		r9, [rbp+28h]
	mov		ebx, dword ptr [r9]
	cmp		ebx, 1
	je		LF
	mov		rdx, qword ptr [rsp];
	jmp		N1
LF:
	movsd	xmm1, mmword ptr [rsp]

N1:
	dec		eax
	cmp		eax, 0
	je		L6
	add		r9, 8
	mov		ebx, dword ptr [r9]
	cmp		ebx, 1
	je		F2
	mov		r8, qword ptr [rsp + 8];
	jmp		N2
F2:
	movsd	xmm2, mmword ptr [rsp + 8]

N2:
	dec		eax
	cmp		eax, 0
	je		L6
	add		r9, 8
	mov		ebx, dword ptr [r9]
	cmp		ebx, 1
	je		F3
	mov		r9, qword ptr [rsp + 10h];
	jmp		L6
F3:
	movsd	xmm3, mmword ptr [rsp + 10h]

L6:
	sub		rsp, 8
	call	qword ptr [rbp+18h]
	
;	mov		rbx, qword ptr [rbp-8]
;	mov		rsi, qword ptr [rbp-16]
;	mov		rdi, qword ptr [rbp-24]

	leave
	ret
__signal_call_marshal ENDP

END
