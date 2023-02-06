/*
Acess OS
Main Header file
HEADER
*/
#ifndef __ACESS_H
#define __ACESS_H

#define	ACESS_PRODUCTION	0

#define HIB(x) 	(((x) & 0xFF00) >> 8)
#define LOWB(x)	((x) & 0x00FF)
#define HIW(x)	(((x) & 0xFFFF0000) >> 16)
#define LOWW(x)	((x) & 0xFFFF)
#define SEG(a)	(((a)>>4)&0xFFF0)
#define OFS(a)	((a)&0x00FF)

#define	CONCAT(x,y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)
#define STR(x) #x
#define EXPAND_STR(x) STR(x)

#define NULL (void *)(0)
#define SEEK_END	(-1)
#define SEEK_CUR	(0)
#define SEEK_SET	(1)

#define VFS_OPENFLAG_READ	1
#define VFS_OPENFLAG_WRITE	2
#define VFS_OPENFLAG_EXEC	4
#define VFS_OPENFLAG_USER	0x10

// Locking functions - Used to protect against data corruption
#define LOCK()	__asm__ __volatile__ ("pushf;cli")
#define UNLOCK()	__asm__ __volatile__ ("popf")

// Array Size - Size of static array
#define SIZEOF_ARR(arr)	(sizeof(arr)/sizeof((arr)[0]))

#define	KB_ACK	(-4)

// Settings
#define IRQ_OFFSET	0xF0
#define ACESSBASIC_UID	1
#define ACESSBASIC_GID	1
//#define TIMER_DIVISOR	1193	//~1kHz
//#define TIMER_DIVISOR	11931	//~100Hz
#define TIMER_DIVISOR	119318	//~10Hz


// ==== Base Types ====
typedef unsigned int	Uint;
typedef unsigned long long	Uint64;
typedef unsigned long	Uint32;
typedef	unsigned short 	Uint16;
typedef	unsigned char	Uint8;
typedef signed int		Sint;
typedef signed long long	Sint64;
typedef	signed long		Sint32;
typedef	signed short	Sint16;
typedef	signed char		Sint8;

// ==== Complex Types ====
// Register State
/**
 \struct regs
 \brief 32-Bit Register State
*/
struct regs {
    Uint	gs, /**< GS */ fs, /**< FS */ es, /**< ES */ ds; /**< DS */
    Uint	edi, /**< EDI */ esi, /**< ESI */ ebp, /**< EBP */ kesp; /**< Kernel ESP */
	Uint	ebx, /**< EBX */ edx, /**< EDX */ ecx, /**< ECX */ eax; /**< EAX */
    Uint	int_no, /**< Interrupt */ err_code; /**< Error code */
    Uint	eip, /**< Pre-Int EIP */ cs; /**< Pre-Int CS */
	Uint	eflags, /**< EFLAGS */ esp, /**< User ESP */ ss; /**< User SS */
};
typedef struct regs	t_regs;	//!< Register State Type

/**
 \struct regs16
 \brief VM8086 Register State
*/
struct regs16 {
	Uint16	ax, /**< AX */ cx, /**< CX */ bx, /**< BX */ dx; /**< DX */
	Uint16	sp, /**< SP */ bp, /**< BP */ di, /**< DI */ si; /**< SI */
	Uint16	ds, /**< DS */ es, /**< ES */ cs, /**< CS */ ss; /**< SS */
	Uint16	ip; /**< IP */
};
typedef struct regs16	tRegs16;	//!< 16-Bit Register State Type

// fstat structure
/**
 \struct s_fstat
 \brief File Information Structure
*/
struct s_fstat {
	int		st_dev;		//!< Device
	int		st_ino;		//!< Inode
	int		st_mode;	//!< Mode
	Uint	st_nlink;	//!< Number of links
	Uint	st_uid;		//!< Owner's UID
	Uint	st_gid;		//!< Owning Group ID
	int		st_rdev;	//!< Rdev?
	Uint	st_size;	//!< Size
	long	st_atime;	//!< Last Accessed Time
	long	st_mtime;	//!< Last Modifcation Time
	long	st_ctime;	//!< Creation Time
};
typedef struct s_fstat	t_fstat;	//!< File Status Type

/* LINK.LD */
extern void	gKernelEnd;	//!< Kernel End Pointer
/* MAIN.C */
extern Uint32	giAcessVersion;	//!< Acess Version Number (Was used for OS vs Basic)
/* LIB.C */
extern const char cHEX_DIGITS[16];	//!< Import from lib.c, hexadecimal digits

/* SYSTEM.C */
extern Uint32 giMemoryCount;	//!< Total Memory Size
extern char	*gsNullString;		//!< Null String ("") (Is it nessasary?)
/**
 \fn void Kernel_KeyEvent( Uint8 *gbaKeystates, Uint8 keycode );
 \brief Kernel Key Handler
 \param gbaKeystates	Keystate Pointer (Boolean Array)
 \param keycode	Key that caused the event
*/
extern void Kernel_KeyEvent( Uint8 *gbaKeystates, Uint8 keycode );

/* LIB.C */
extern void *memcpy(void *dest, const void *src, Uint count);
extern void memcpyd(void *dst, void *src, Uint dwCount);
extern void memcpyda(void *dst, void *src, Uint dwCount);
extern void *memset(void *dest, char val, Uint count);
extern void *memsetw(void *dest, Uint16 val, Uint count);
extern void *memsetd(void *dest, Uint32 val, Uint count);
#define memsetwa(dst, val, num)	__asm__ __volatile__ ("pushf;cld;rep;stosw;popf;" :: "c"((num)), "a"((val)), "D"((dst)))
extern void memsetda(void *dest, Uint32 val, Uint count);
// String Manipulation
extern int	strlen(const char *str);
extern int	strcmp(const char *str1, const char *str2);
extern int	strncmp(const char *str1, const char *str2);
extern void	strcpy(char *dest, const char *src);
// atoi
extern Uint	HexToInt(char *str);
extern Uint	DecToInt(char *str);
extern Uint	OctToInt(char *str);
extern Uint	BinToInt(char *str);
// Port IO
extern Uint8	inportb (Uint16 _port);
extern Uint16	inportw (Uint16 _port);
extern Uint32	inportd (Uint16 _port);
extern void outportb (Uint16 _port, Uint8 _data);
extern void outportw (Uint16 _port, Uint16 _data);
extern void outportd (Uint16 _port, Uint32 _data);
// Linked List Maipulation
extern void *ll_findId(void **start, int nextOfs, int idOfs, int id);
extern void ll_delete(void **start, int nextOfs, void *item);
extern void ll_append(void **start, int nextOfs, void *item);

/* DEBUG.C */
extern void Debug_LoadELFSymtab(Uint32 num, Uint32 spe, Uint32 addr, Uint32 shndx);
extern char *Debug_GetSymbol(Uint addr, Uint *delta);
extern void Debug_Backtrace(Uint eip, Uint ebp);
extern void Debug_Disasm(Uint eip);

/* SCRN.C */
extern void VGAText_Install(void);
extern void putch(char c);
extern void puts(char *text);
extern void putsa(char attr, char *text);
extern void VGAText_SetAttrib(Uint8 att);
extern void scrn_cls();
extern void putHex(unsigned long number);
extern void putNum(unsigned long number);
extern void putNumS(long number);
extern void LogF(const char *format, ...);
#define printf(v...)	LogF(v)
extern void panic(char *msg, ...);
extern void warning(char *msg, ...);

enum {
	CLR_BLACK,	//0x0
	CLR_BLUE,
	CLR_GREEN,
	CLR_CYAN,
	CLR_RED,
	CLR_MAGENTA,
	CLR_YELLOW,
	CLR_GREY,	//0x7
};
#define CLR_FLAG	0x8

/* GDT.C */
extern void gdt_set_gate(int num, Uint32 base, Uint32 limit, Uint8 access, Uint8 gran);
extern void gdt_install();

/* IDT.C */
extern void IDT_SetGate(Uint8 num, Uint32 base, Uint16 sel, Uint8 flags);
extern void IDT_Install();

/* ISRS.C */
extern void isrs_install();	//*/

/* IRQ.C */
extern void irq_install_handler(int irq, void (*handler)(struct regs *r));
extern void irq_uninstall_handler(int irq);
extern void irq_install(); //*/

/* TIME.C */
extern void time_wait(int ticks);
extern void time_install(); //*/
extern int	now();
extern Uint64	unow();
extern int	timestamp(int sec, int mins, int hrs, int day, int month, int year);
extern int	time_createTimer(int delay, void(*callback)(int), int arg, int oneshot);
extern void	time_removeTimer(int id);
extern void Timer_Disable();

/* DISK.C */
extern void setup_disks();
extern int	disk_read(Uint8 disk, int offset, int length, void *buffer);

/* VFS.C */
extern void	vfs_install();
extern int	vfs_mount(char *device, char *mountPoint, Uint16 filesystem);
extern int	vfs_open(char *path, int flags);
extern int	vfs_opendir(char *path);
extern int	vfs_readdir(int dp, char *file);
extern int	vfs_read(int fileHandle, int length, void *buffer);
extern int	vfs_write(int fileHandle, int length, void *buffer);
extern void vfs_seek(int fileHandle, int distance, int flag);
extern int	vfs_tell(int fileHandle);
extern void	vfs_close(int fileHandle);
extern int	vfs_stat(int fp, t_fstat *info);
extern char	*VFS_GetTruePath(char *path);
extern int	vfs_mknod(char *path, int isdir);
extern int	vfs_ioctl(int fp, int id, void *data);

/* HEAP.C */
extern void	Heap_Init();
extern void	*malloc(Uint32 bytes);
extern void	*realloc(Uint32 bytes, void *orig);
extern void	free(void *mem);
extern void *AllocLocal(Uint32 bytes);
extern void DeallocLocal(void *mem);

/* PROC.C */
extern int	proc_pid;
extern int	Proc_Open(char *filename, char **argv, char **envp);
extern int	Proc_Fork();
extern int	Proc_Execve(char *path, char **argv, char **envp, int dpl);
extern void	Proc_Sleep();
extern void	Proc_Yield();
extern void	Proc_Delay(int ms);
extern void	Proc_Exit();
extern int	Proc_GetUid();
extern int	Proc_GetGid();
extern int	Proc_WaitPid(int pid, int action);
extern Uint32 proc_brk(Uint32 next);
/* MM.C/PROC.C */
extern void	*MM_AllocRange(int rangeMsn, int count);
extern void	MM_DeallocRange(void *ptr, int count);
extern void *MM_MapHW(Uint addr, int count);
extern void	MM_UnmapHW(void *ptr, int count);
extern void	mm_dealloc(Uint32 addr);
extern Uint32 mm_alloc(Uint32 addr);
extern int	MM_IsValid(Uint address);
extern int	MM_MapEx(Uint vaddr, Uint ppage, int flags);
extern Uint	MM_GetPhysAddr(Uint vaddr);
extern void	*MM_AllocDMA(int pages, Uint *paddr);

/* DMA.C */
extern int	dma_readData(int channel, int count, void *buffer);
extern void dma_setChannel(int channel, int length, int read);

/* DRV_PCI.C */
extern int	PCI_CountDevices(Uint16 vendor, Uint16 device, Uint16 function);
extern int	PCI_GetDevice(Uint16 vendor, Uint16 device, Uint16 function, int idx);
extern Uint8	PCI_GetIRQ(int id);
extern Uint32	PCI_GetBAR0(int id);
extern Uint32	PCI_GetBAR1(int id);
extern Uint16	PCI_AssignPort(int id, int bar, int count);

/* MODULE.C */
extern int	Module_Load(char *file);

/* VM8086.C */
extern void VM8086_Lock();
extern void VM8086_Unlock();
extern void *VM8086_Allocate(int bytes);
extern int VM8086_Int(int id, tRegs16 *r);
extern void *VM8086_Linear(Uint16 seg, Uint16 ofs);

/* MISC DRIVERS */
extern void Keyboard_Install();
extern int	PS2Mouse_Install();
extern void Syscall_Install();
extern int	tty_install();
extern int	BGA_Install();
extern int	Vesa_Install();
extern void	PCI_Install();
extern int	RTL8139_Install();

/* DEBUG DUMPERS */
#if !defined(ACESS_PRODUCTION) || !ACESS_PRODUCTION
//extern void	DynLib_DumpLoaded();
#endif

// Timer Information
#define TIMER_BASE	1193182 //Hz
#define MS_PER_TICK_WHOLE	(1000*(TIMER_DIVISOR)/(TIMER_BASE))
#define MS_PER_TICK_FRACT	((1000*0x100000000*(TIMER_DIVISOR)/(TIMER_BASE))&0xFFFFFFFF)
#define TICKS_PER_MS	((TIMER_BASE)/(TIMER_DIVISOR)/1000)

//Flags
#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

#endif
