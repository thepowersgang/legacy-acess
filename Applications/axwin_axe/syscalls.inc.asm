;AcessOS Basic C Library
; System Calls
; SYSCALLS.INC.ASM


;[local SYS_NULL]
;[local SYS_EXIT]

SYS_NULL	equ	0
SYS_EXIT	equ	1
SYS_OPEN	equ	2
SYS_CLOSE	equ	3
SYS_READ	equ	4
SYS_WRITE	equ	5
SYS_SEEK	equ	6
SYS_TELL	equ	7
SYS_UNLINK	equ	8
SYS_GETPID	equ	9
SYS_KILL	equ	10
SYS_FSTAT	equ	11

SYS_READDIR	equ	12
SYS_IOCTL	equ	13
	
SYS_WAIT	equ	14
SYS_KEXEC	equ	15
SYS_FORK	equ	16
SYS_EXECVE	equ	17
SYS_WAITPID	equ	18
SYS_BRK		equ	19


%macro SYSCALL0	1
	mov eax, %1
	int	0xAC
%endmacro

%macro SYSCALL1	1
	push ebx
	mov ebx, [esp+0x8]
	mov eax, %1
	int	0xAC
	pop ebx
%endmacro

%macro SYSCALL2	1
	push ebx
	mov ebx, [esp+0x8]
	mov ecx, [esp+0xC]
	mov eax, %1
	int	0xAC
	pop ebx
%endmacro

%macro SYSCALL3	1
	push ebx
	push edx
	mov ebx, [esp+0xC]
	mov ecx, [esp+0x10]
	mov edx, [esp+0x14]
	mov eax, %1
	int	0xAC
	pop edx
	pop ebx
%endmacro

