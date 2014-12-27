.386P

_TEXT	SEGMENT	'CODE'

___signal_call_marshal PROC NEAR
	push	ebp
	mov		ebp, esp
	sub		esp, 4 
	mov   	esi, 16[ebp]
	mov		ebx, 20[ebp]
	mov		edx, 24[ebp]
	sub		esp, 28[ebp]
	lea		edi,  4[esp]
L0:
	cmp     edx, 0
	je      L5
	mov     eax, [ebx]
	cmp     eax, 1
	je      L3
	mov     ecx,4[ebx]
	add     ecx, 3
	shr		ecx, 1
	shr		ecx, 1
L2:
	mov		eax, [esi]
	mov		[edi], eax
	add		esi, 4
	add		edi, 4
	loop	L2
	jmp		L4
L3:
	fld		QWORD PTR [esi]
	mov		ecx, 4[ebx]
	cmp		ecx, 8
	je		_DF
	fstp	DWORD PTR [edi]
	add		edi, 4
	jmp		_DD
_DF:
	fstp	QWORD PTR [edi]
	add		edi, 8
_DD:
	add		esi, 8
L4:
	add		ebx, 8
	dec		edx
	jne		L0
L5:
	mov		eax, 8[ebp]
	mov		[esp], eax
	call	DWORD PTR 12[ebp]
	leave
	ret
___signal_call_marshal ENDP

___signal_call_marshal_1 PROC NEAR
	push	ebp
	mov		ebp, esp
	sub		esp, 8
	mov   	esi, 20[ebp]
	mov		ebx, 24[ebp]
	mov		edx, 28[ebp]
	sub		esp, 32[ebp]
	lea		edi,  8[esp]
P0:
	cmp     edx, 0
	je      P5
	mov     eax, [ebx]
	cmp     eax, 1
	je      P3
	mov     ecx,4[ebx]
	add     ecx, 3
	shr		ecx, 1
	shr		ecx, 1
P2:
	mov		eax, [esi]
	mov		[edi], eax
	add		esi, 4
	add		edi, 4
	loop	P2
	jmp		P4
P3:
	fld		QWORD PTR [esi]
	mov		ecx, 4[ebx]
	cmp		ecx, 8
	je		_DF1
	fstp	DWORD PTR [edi]
	add		edi, 4
	jmp		_DD1
_DF1:
	fstp	QWORD PTR [edi]
	add		edi, 8
_DD1:
	add		esi, 8
P4:
	add		ebx, 8
	dec		edx
	jne		P0
P5:
	mov		eax, 16[ebp]
	mov		4[esp], eax
	mov		eax, 8[ebp]
	mov		[esp], eax
	call	DWORD PTR 12[ebp]
	leave
	ret
___signal_call_marshal_1 ENDP

_TEXT	ENDS
END
