/*
System Calls
HEADER
*/
#ifndef __SYSCALLS_H
#define __SYSCALLS_H

//Function Defintions
extern int open(char *file, int flags, int mode);
extern int close(int fp);
extern int read(int fp, int len, void *buf);
extern int write(int fp, int len, void *buf);
extern int tell(int fp);
extern void seek(int fp, int dist, int flag);
extern int readdir(int fp, char *file);
extern int brk(int bssend);

#endif
