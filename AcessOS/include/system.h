/*
Acess OS
Main Header file
HEADER
*/
#ifndef __SYSTEM_H
#define __SYSTEM_H

#include <acess.h>

#define PROC_MAX_PID	2048
#define MAX_CPUS		1

#define KHEAP_START		0xC0400000
#define	USERMEM_START	0x00200000
#define	KSTACK_ADDR		0xEFFF0000
#define	KSTACK_SIZE		0x00010000

extern int	sys_stack_start;
extern int	sys_stack;

/* MEMMANAGER.C */
extern void *MM_AllocPhys();
extern void freephys(void *mem);
extern int	mm_init(void);
extern void mm_map(Uint vaddr, Uint page);
extern int	MM_MapEx(Uint vaddr, Uint ppage, int flags);
extern void mm_setro(Uint32 vaddr, int ro);
extern Uint32 mm_alloc(Uint32 vaddr);
extern void mm_dealloc(Uint32 vaddr);
extern void mm_remove(int pid);
extern int	MM_CloneCurrent(void);
extern void MM_Clear();
extern void MM_Dump();
extern void memmanager_install(Uint32 memory);
extern void paging_enable();
extern void paging_disable();
extern void paging_setpid(int pid);
extern void MM_SetPid(int pid);
extern int	MM_CloneKernel();
extern Uint	MM_MapTemp(Uint paddr);
extern void	MM_FreeTemp(Uint addr);

/* PROC.C */
extern void Proc_Install();
extern int	Proc_Fork();
extern int	Proc_KFork();
extern int	Proc_Kill(int pid);
extern int	Proc_Execve(char *path, char **argv, char **envp, int dpl);
extern void	proc_close(int pid);
extern int	proc_getcurpid();
extern Uint32 proc_brk(Uint32 newEnd);
extern void Proc_WakeProc(int pid);

/* VM8086.C */
extern void VM8086_Install();
extern void VM8086_GPF(t_regs *r);

#endif
