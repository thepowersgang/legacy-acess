; AcessOS v1
; User.asm
; Helper Usermode code

[bits 32]

[section .utext]

[global _gUserCode]
[global _rUser_ReturnExit]


_gUserCode:

_rUser_Return1:
	xchg bx, bx
	mov edx, eax	; Save Return Value
	pop eax	; Arg1
	pop eax	; Caller
	pop ecx	; ESP
	mov esp, ecx	; Restore ESP
	mov eax, 0x101	; Set Call Number
	int 0xAC	; System Call
_rUser_Return0:
	xchg bx, bx
	mov edx, eax	; Save Return Value
	pop eax	; Caller
	pop ecx	; ESP
	mov esp, ecx	; Restore ESP
	mov eax, 0x101	; Set Call Number
	int 0xAC	; System Call

_rUser_ReturnExit:
	mov ebx, eax	; Save Return
	mov eax, 0x1	; SYS_EXIT
	int 0xAC	; System Call
	