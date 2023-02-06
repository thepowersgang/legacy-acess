/*
 Acess Shell Version 2
- Based on IOOS CLI Shell
*/
#ifndef _HEADER_H
#define _HEADER_H

#define NULL	((void*)0)

#define Print(str)	do{char*s=(str);write(_stdout,strlen(s)+1,s);}while(0)

extern int _stdout;
extern int _stdin;

extern int	strcmp(char *s1, char *s2);
extern void	strcpy(char *dst, char *src);
extern int	strlen(char *str);
extern void	printf(char *fmt, ...);
extern int	GeneratePath(char *file, char *base, char *tmpPath);


#endif
