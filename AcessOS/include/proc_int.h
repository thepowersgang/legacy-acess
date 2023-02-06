/*
AcessOS 1
Process Manager - Internal Header
*/
#ifndef _PROC_INT_H
#define _PROC_INT_H

#include <proc.h>

struct s_tss {
	Uint32	previous;	//Backwards Linked List (For HW Multitasking)
	Uint32	esp0, ss0;	//KMode Stack Pointer and Segment
	Uint32	esp1, ss1;	//Ring 1  "
	Uint32	esp2, ss2;	//Ring 2  "
	Uint32	cr3;		//CR3 Value for HW Multitasking
	Uint32	eip;		//ESP (HW)
	Uint32	eflags;		//EFLAGS (HW)
	Uint32	eax, ecx, edx, ebx;	//(HW)
	Uint32	esp, ebp, esi, edi;	//(HW)
	Uint32	es, cs, ss, ds, fs, gs;	//Segment Values for kernel mode (SS again?)
	Uint32	ldt;		//LDT Pointer for each task? (HW)
	Uint16	trace, iobitmap;
} __attribute__((packed));
typedef struct s_tss	 t_proc_tss;

enum {
	PROCSTAT_NULL,
	PROCSTAT_ACTIVE,
	PROCSTAT_SLEEP
};

#define PROC_FLAG_VM	0x1
#define PROC_FLAG_VMINT	0x3

#define PROC_STATUS_NULL	0x0
#define PROC_STATUS_ACTIVE	0x1
#define PROC_STATUS_SLEEP	0x2

typedef struct proc_process {
	Uint32	esp, ebp, eip;
	int		slots;	// 0x10
	int		timeRem;
	int		status;
	int		pid, ppid;	// 0x20
	Uint	uid, gid;
	int		paging_id;
	Uint	endBss;	// 0x30
	Uint	flags;	// 0x34
	struct proc_process	*next;
	struct proc_process	*Waiting;
	char	*ImageName;
} t_proc;

typedef struct proc_wait {
	Uint	delta;
	t_proc	*proc;
	struct proc_wait	*next;
} t_procWait;

enum {
	PROC_WAITPROC_DIE,
	PROC_WAITPROC_LAST
};

#endif
