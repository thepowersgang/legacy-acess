/*
 AcessOS v1
 By thePowersGang
 ld-acess.so
 COMMON.H
*/
#ifndef _COMMON_H
#define _COMMON_H

#define	NULL	((void*)0)

// === Types ===
typedef unsigned int	Uint;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef signed char		Sint8;
typedef signed short	Sint16;
typedef signed long		Sint32;

// === Main ===
extern int	DoRelocate( Uint base, char **envp );

// === Library/Symbol Manipulation ==
extern int	LoadLibrary(char *filename, char **envp);
extern void	AddLoaded(Uint base);
extern Uint	GetSymbol(char *name);

// === Library Functions ===
extern void	strcpy(char *dest, char *src);
extern int	strcmp(char *s1, char *s2);

// === System Calls ===
extern void	SysExit();
extern void	SysDebug(char *fmt, ...);
extern Uint	SysLoadBin(char *path, Uint *entry);

// === ELF Loader ===
extern int	ElfGetSymbol(Uint Base, char *name, Uint *ret);

#endif
