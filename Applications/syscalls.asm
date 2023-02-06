; System Call Prototypes
;


[bits 32]

%define	SYSCALL_INT_NO	0xAC

%macro  mpush 1-* 
	%rep	%0 
	 push	%1 
	%rotate	1 
	%endrep
%endmacro
%macro  mpop 1-* 
	%rep	%0 
	 pop	%1 
	%rotate	1 
	%endrep
%endmacro


%macro SYSCALL0	1
	mov eax, %1
	push ebx
	int	SYSCALL_INT_NO
	mov DWORD [_kerrno], ebx
	pop ebx
%endmacro
%macro SYSCALL1	1
	push ebp
	mov ebp, esp
	push ebx
	mov eax, %1
	mov ebx, [ebp+8]
	int	SYSCALL_INT_NO
	mov DWORD [_kerrno], ebx
	pop ebx
	pop ebp
%endmacro
%macro SYSCALL2	1
	push ebp
	mov ebp, esp
	mpush ebx
	mov eax, %1
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	int	SYSCALL_INT_NO
	mov DWORD [_kerrno], ebx
	mpop ebx
	pop ebp
%endmacro
%macro SYSCALL3	1
	push ebp
	mov ebp, esp
	mpush ebx, edx
	mov eax, %1
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	int	SYSCALL_INT_NO
	mov DWORD [_kerrno], ebx
	mpop edx, ebx
	pop ebp
%endmacro

[global _open]
[global _close]
[global _read]
[global _write]
[global _stat]
[global _tell]
[global _seek]
[global _readdir]
[global _kexec]
[global _ioctl]
[global _opendir]
[global _waitpid]
[global _fork]
[global _yield]
[global _kdebug]

_open:
	SYSCALL3	2
	ret
_close:
	SYSCALL1	3
	ret
_read:
	SYSCALL3	4
	ret
_write:
	SYSCALL3	5
	ret
_seek:
	SYSCALL3	6
	ret
_tell:
	SYSCALL1	7
	ret
_stat:
	SYSCALL2	11
	ret
_readdir:
	SYSCALL2	12
	ret
_ioctl:
	SYSCALL3	13
	ret
_kexec:
	SYSCALL3	15
	ret
_waitpid:
	SYSCALL2	18
	ret

_fork:
	SYSCALL0	16
	ret

_yield:
	SYSCALL0	21
	ret
	
_kdebug:
	SYSCALL3	256
	ret

_opendir:
	push 0
	push 0
	push DWORD [esp+12]
	call _open
	add esp, 12
	ret

[section .data]
_kerrno:	dw	0
	