; AcessBasic Test Command Prompt
; Assembler Stub

[BITS 32]

[extern _main]
[global start]

;Code
start:
	call _main
	mov eax, 1	; SYS_EXIT
	int 0xAC
	ret

