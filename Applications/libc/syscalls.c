/*
AcessOS System Call Interface
*/
#include <syscalls.h>

//SYSCALL Functions
unsigned int SYSCALL0(int num) {
	unsigned int ret;
	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num));
	return ret;
}
unsigned int SYSCALL1(int num, unsigned int arg1) {
	unsigned int ret;
	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1));
	return ret;
}
unsigned int SYSCALL2(int num, unsigned int arg1, unsigned int arg2) {
	unsigned int ret;
	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1), "c" (arg2));
	return ret;
}
unsigned int SYSCALL3(int num, unsigned int arg1, unsigned int arg2, unsigned int arg3) {
	unsigned int ret;
	__asm__ __volatile__ ("int $0xAC" : "=a" (ret) : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3));
	return ret;
}

//Syscall Methods
int open(char *file, int flags, int mode)
{
	return SYSCALL3(SYS_OPEN, (unsigned int)file, flags, mode);
}

int close(int fp)
{
	return SYSCALL1(SYS_CLOSE, fp);
}

int read(int fp, int len, void *buf)
{
	return SYSCALL3(SYS_READ, fp, len, (unsigned int)buf);
}

int write(int fp, int len, void *buf)
{
	return SYSCALL3(SYS_WRITE, fp, len, (unsigned int)buf);
}

void seek(int fp, int dist, int flag)
{
	SYSCALL3(SYS_SEEK, fp, dist, flag);
}

int tell(int fp)
{
	return SYSCALL1(SYS_SEEK, fp);
}

int readdir(int fp, char *file)
{
	return SYSCALL2(SYS_READDIR, fp, (unsigned int)file);
}

int brk(unsigned int bssend)
{
	return SYSCALL1(SYS_BRK, bssend);
}
