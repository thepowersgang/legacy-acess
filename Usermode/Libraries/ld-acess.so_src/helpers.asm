; AcessOS 1
; By thePowersGang
; LD-ACESS.SO
; - helpers.asm

[global _SysDebug]
[global _SysExit]
[global _SysLoadBin]

; void SysDebug(char *fmt, ...)
_SysDebug:
	;xchg bx, bx
	push ebx
	push edi
	push esi
	mov eax, 0x100	; User Debug
	mov ebx, [esp+0x10]	; Format
	mov ecx, [esp+0x14]	; Arg 1
	mov edx, [esp+0x18]	; Arg 2
	mov edi, [esp+0x1C]	; Arg 2
	mov esi, [esp+0x20]	; Arg 2
	int	0xAC
	pop edi
	pop esi
	pop ebx
	ret

; void SysExit()
_SysExit:
	push ebx
	mov eax, 1	; Exit
	int	0xAC
	pop ebx
	ret

; Uint SysLoadBin(char *path, Uint *entry)
_SysLoadBin:
	push ebx
	mov eax, 20	; SYS_LDBIN
	mov ebx, [esp+0x8]
	mov ecx, [esp+0xC]
	int	0xAC
	pop ebx
	ret
