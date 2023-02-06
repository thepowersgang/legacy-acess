/*
AcessOS LibC

stdlib.h
*/
#ifndef __STDLIB_H
#define __STDLIB_H

#include <va_args.h>

typedef unsigned int	size_t;

extern void printf(char *format, ...);//	__asm__ ("printf");

extern void sprintfv(char *buf, char *format, va_list args);//	__asm__ ("sprintfv");
extern int ssprintfv(char *format, va_list args);//	__asm__ ("ssprintfv");
extern void sprintf(char *buf, char *format, ...);//	__asm__ ("sprintf");
extern void *memcpy(void *dest, void *src, unsigned int count);//	__asm__ ("memcpy");

extern void free(void *mem);//	__asm__ ("free");
extern void *malloc(unsigned int bytes);//	__asm__ ("malloc");
extern void *realloc(unsigned int bytes, void *oldPos);//	__asm__ ("realloc");

//syscall stat structure
typedef struct {
	int		st_dev;		//dev_t
	int		st_ino;		//ino_t
	int		st_mode;	//mode_t
	unsigned int	st_nlink;
	unsigned int	st_uid;
	unsigned int	st_gid;
	int		st_rdev;	//dev_t
	unsigned int	st_size;
	long	st_atime;	//time_t
	long	st_mtime;
	long	st_ctime;
} t_fstat;
#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

#define	OPEN_FLAG_READ	1
#define	OPEN_FLAG_WRITE	2
#define	OPEN_FLAG_EXEC	4

enum {
	K_WAITPID_DIE = 0
};

extern int	open(char *file, int flags, int mode);//	__asm__ ("open");
extern int	close(int fp);//	__asm__ ("close");
extern int	read(int fp, int len, void *buf);//	__asm__ ("read");
extern int	write(int fp, int len, void *buf);//	__asm__ ("write");
extern int	tell(int fp);//	__asm__ ("tell");
extern void seek(int fp, int dist, int flag);//	__asm__ ("seek");
extern int	stat(int fp, t_fstat *st);//	__asm__ ("stat");
extern int	ioctl(int fp, int call, void *arg);//	__asm__ ("ioctl");
extern int	readdir(int fp, char *file);//	__asm__ ("readdir");
extern int	brk(int bssend);//	__asm__ ("brk");
extern int	kexec(char *file, char *args[], char *envp[]);//	__asm__ ("kexec");
extern int	waitpid(int pid, int action);//	__asm__ ("waitpid");


#endif
