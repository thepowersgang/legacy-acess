/*
AcessOS Version 1
Interface Layer Common
SYSCALLS.H
*/
#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#define	SYSCALL0(ret, num)	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num))
#define	SYSCALL1(ret, num, arg1)	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1) )
#define	SYSCALL2(ret, num, arg1, arg2)	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1), "c" (arg2) )
#define	SYSCALL3(ret, num, arg1, arg2, arg3)	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3) )

enum SyscallNumbers {
	SYS_NULL,	//0
	SYS_EXIT,	//1
	SYS_OPEN,	//2
	SYS_CLOSE,	//3
	SYS_READ,	//4
	SYS_WRITE,	//5
	SYS_SEEK,	//6
	SYS_TELL,	//7
	SYS_UNLINK,	//8
	SYS_GETPID,	//9
	SYS_KILL,	//10
	SYS_FSTAT,	//11
	SYS_READDIR,//12
	SYS_IOCTL,	//13
	SYS_WAIT,	//14
	SYS_KEXEC,	//15
	SYS_FORK,	//16
	SYS_EXECVE,	//17
	SYS_WAITPID,//18
	SYS_SBRK	//19
};

static inline int	_write(int fp, int len, char *string)
{
	int ret;
	SYSCALL3(ret, SYS_WRITE, fp, len, string);
	return ret;
}

#endif
