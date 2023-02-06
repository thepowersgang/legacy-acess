; AcessOS Basic C Library
;
; SYSCALLS.ASM
[BITS 32]

%include "syscalls.inc.asm"

[section .text]

global _open:function
global _close:function
global _read:function
global _write:function
global _seek:function
global _tell:function
global _readdir:function
global _brk:function
global _stat:function
global _ioctl:function

; === Actual System Calls ===
_open:
	SYSCALL3	SYS_OPEN
	ret
_close:
	SYSCALL1	SYS_CLOSE
	ret
_read:
	SYSCALL3	SYS_READ
	ret
_write:
	SYSCALL3	SYS_WRITE
	ret
_seek:
	SYSCALL3	SYS_SEEK
	ret
_tell:
	SYSCALL1	SYS_TELL
	ret
_readdir:
	SYSCALL2	SYS_READDIR
	ret
_brk:
	SYSCALL1	SYS_BRK
	ret
_stat:
	SYSCALL2	SYS_FSTAT
	ret
_ioctl:
	SYSCALL3	SYS_IOCTL
	ret
