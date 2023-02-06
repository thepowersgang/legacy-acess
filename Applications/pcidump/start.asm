; AcessBasic Test Command Prompt
; Assembler Stub

[BITS 32]

extern _main
extern codeLength
extern loadTo
extern entrypoint
extern magic

[global start]
[global _memset]
[global __stdin]
[global __stdout]

FLAGS equ 0
MAXMEM equ 0

;Header
db	'A'
db	'X'
db	'E'
db	0
;Size
dd codeLength	;Code Size
dd loadTo		;Load Address
;dd entrypoint	;Entrypoint
dd MAXMEM	;Used Memory = 4096k
dd FLAGS	;Flags
;dd magic+FLAGS+MAXMEM

;Code
start:
	mov eax, 2	;SYS_OPEN
	mov ebx, termPath
	mov ecx, 3	; Read(1) and Write(2)
	mov edx, 0
	int	0xAC
	cmp eax, -1
	jz	.stdioBad
	mov	[__stdout], eax
	mov [__stdin], eax
	
	call _main	
	mov [tmpRet], eax

	mov eax, 3	;SYS_CLOSE
	mov ebx, [__stdout]
	int	0xAC

	mov	ebx, [tmpRet]
	mov eax, 1	;  SYS_EXIT
	int 0xAC
	ret
.stdioBad:
	mov ebx, 0xBAD10
	mov eax, 1	;  SYS_EXIT
	int 0xAC
	ret
	
;Memory Set
_memset:
	push ebp
	mov ebp, esp
	push edi
	
	mov edi, DWORD[ebp+0x8]
	mov eax, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep stosb
	
	pop edi
	pop ebp
	ret

[section .data]
termPath:	db "/Devices/vterm/1",0
__stdout:	dd 0
__stdin:	dd 0
tmpRet:		dd 0
