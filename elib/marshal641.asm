.CODE
;IN_OBJ		rbp+10h	rcx
;IN_FN		rbp+18h	rdx
;IN_DATA	rbp+20h	r8
;IN_VA		rbp+28h	r9
;IN_AN		rbp+30h
;IN_NUM		rbp+38h
;IN_SIZE	rbp+40h

__signal_call_marshal_1 PROC
	push	rbp
	mov		rbp, rsp
;	sub		rsp, 24

;	mov		qword ptr [rbp-8], rbx
;	mov		qword ptr [rbp-16], rsi
;	mov		qword ptr [rbp-24], rdi

	mov		qword ptr [rbp+10h], rcx
	mov		qword ptr [rbp+18h], rdx
	mov		qword ptr [rbp+20h], r8

	mov		ecx, dword ptr [rbp+38h]
	mov		eax, ecx
	mov		ebx, 8
	mul		ebx
	sub		rsp, rax
	and		rsp, 0fffffffffffffff0h
	mov		rsi, r9
	mov		rdi, rsp
	mov		rdx, [rbp+30h]

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
	mov		rdx, qword ptr [rbp+20h]

	mov		eax, dword ptr [rbp+38h]
	mov		rbx, rsp
	cmp		eax, 0
	je		L6
	mov		r9,  qword ptr [rbp+30h]
	mov		ebx, dword ptr [r9]
	cmp		ebx, 1
	je		LF
	mov		r8, qword ptr [rsp];
	jmp		N1
LF:
	movsd	xmm2, mmword ptr [rsp]

N1:
	dec		eax
	cmp		eax, 0
	je		L6
	add		r9, 8
	mov		ebx, dword ptr [r9]
	cmp		ebx, 1
	je		F2
	mov		r9, qword ptr [rsp + 8];
	jmp		L6
F2:
	movsd	xmm3, mmword ptr [rsp + 8]

L6:
	sub		rsp, 10h
	call	qword ptr [rbp+18h]
	
;	mov		rbx, qword ptr [rbp-8]
;	mov		rsi, qword ptr [rbp-16]
;	mov		rdi, qword ptr [rbp-24]

	leave
	ret
__signal_call_marshal_1 ENDP

END
