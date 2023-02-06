/*
AcessOS v0.2
Memory Manager v2

====
Memory Layout:
 0x00000000 - 0xBFFFFFFF
  User Space
 0xC0000000 - 0xC3FFFFFF
  Kernel Base Range - Kernel Image and Global Data
 0xC4000000 - 0xC7FFFFFF
  Kernel File Pointers
 0xC8000000 - 0xDFFFFFFF
  Kernel Modules - Drivers etc.
 0xE0000000 - 0xE3FFFFFF
  Process Unique Kernel Data
 0xE4000000 - 0xEBFFFFFF
  User-Mode File Handles
 0xEC000000 - 0xEFFFFFFF
  Kernel Stack
 0xF0000000 - 0xF1FFFFFF
  Paging Information - (Directories and Fractal Maps)
 0xF2000000 - 0xFFFF7FFF
  Memory Mapped Hardware
 0xFFFF8000 - 0xFFFFEFFF
  Tempoary Mappings
 0xFFFFF000 - 0xFFFFFFFF
  Kernel Usermode Code
====
*/
#include <system.h>

#define MM_DEBUG	0
#define DEBUG	(MM_DEBUG | ACESS_DEBUG)

#define DUMP_ON_PF	0

#define KERNEL_BASE	0xC0000000	//124Mb
# define KERNEL_DIR	((KERNEL_BASE) >> 22)

#define REF_ADDR	0xC7C00000	// 4Mb
# define REF_DIR	((REF_ADDR) >> 22)
# define REF_PAGE	(((REF_ADDR) >> 12)&0x3FF)

#define DRIVER_ADDR	0xC8000000	// 384Mb
# define DRIVER_DIR	((DRIVER_ADDR) >> 22)

#define PPDATA_BASE	0xE0000000	// 256Mb
# define PPDATA_DIR	((PPDATA_BASE) >> 22)

#define KSTACK_DIR	((KSTACK_ADDR) >> 22)

#define DIR_ADDR	0xF0000000	// 8Mb - 2048 Processes
# define DIR_DIR	((DIR_ADDR) >> 22)

#define	TABLE_ADDR	0xF0800000	// 4Mb
# define TABLE_DIR	((TABLE_ADDR) >> 22)

#define	TABLE_T_ADDR	0xF0C00000	//4Mb
# define TABLE_T_DIR	((TABLE_T_ADDR) >> 22)

#define MEMMAP_ADDR	0xF2000000		// 224Mb
# define MEMMAP_DIR	((MEMMAP_ADDR) >> 22)

#define	TEMPORARY_ADDR	0xFFFF8000	// 7 Pages
# define TEMPORARY_DIR	((TEMPORARY_ADDR) >> 22)
# define TEMPORARY_PAGE	(((TEMPORARY_ADDR) >> 12)&0x3FF)

#define	USERMODE_ADDR	0xFFFFF000	// 1 KiB
# define USERMODE_DIR	((USERMODE_ADDR) >> 22)
# define USERMODE_PAGE	(((USERMODE_ADDR) >> 12)&0x3FF)

#define REF_PAGE_COUNT	(((2<<20)*sizeof(*giRefCount))>>12)
#define	MAX_DIRS	(((DIR_ADDR)-(TABLE_ADDR))>>12)

#define	FLAG_PRESENT	0x001
#define	FLAG_WRITEABLE	0x002
#define	FLAG_USERMODE	0x004
#define	FLAG_RESERVED1	0x008
#define	FLAG_RESERVED2	0x010
#define	FLAG_ACCESSED	0x020
#define	FLAG_WRITTEN	0x040
#define	FLAG_RESERVED3	0x080
#define	FLAG_RESERVED4	0x100	//0001 00000000b
#define	FLAG_DISKPAGED	0x200	//0010 00000000b
#define	FLAG_CPYONWRITE	0x400	//0100 00000000b
#define	FLAG_SAVEDRW	0x800	//1000 00000000b

#define TABLEPTR(__dir, __page)	((Uint32*)((TABLE_ADDR)+(__dir)*4096+(__page)*4))
#define TABLE(__dir, __page)	(*TABLEPTR(__dir, __page))
#define TABLETMPPTR(__dir, __page)	((Uint32*)((TABLE_T_ADDR)+(__dir)*4096+(__page)*4))
#define TABLETMP(__dir, __page)	(*TABLETMPPTR(__dir, __page))
#define DIRPTR(__pid,__dir)	((Uint32*)(DIR_ADDR+(__pid)*4096+(__dir)*4))
#define DIR(__pid,__dir)	(*DIRPTR(__pid, __dir))
#define	DIRA(__addr)	(*(Uint32*)((TABLE_ADDR)+((TABLE_ADDR)>>10)+((__addr)>>22)*4))
#define	TABLEA(__addr)	(*(Uint32*)((TABLE_ADDR)+((__addr)>>12)*4))
#define	DIRTA(__addr)	(*(Uint32*)((TABLE_T_ADDR)+((TABLE_T_ADDR)>>10)+((__addr)>>22)*4))
#define	TABLETA(__addr)	(*(Uint32*)((TABLE_T_ADDR)+((__addr)>>12)*4))

#define	INVLPG(__addr)	__asm__ __volatile__ ("invlpg (%0)" : : "r" (__addr) );

//start.asm
extern Uint32	*page_dir_kernel;
extern Uint32	*page_table_root;
extern Uint32	*page_table_stack;
extern Uint32	*page_table_kernel;
extern Uint32	*a_page_directories;
extern void	*heap_end;
extern void	gUserCode;
extern int	proc_pid;
Uint32	*page_directories = (Uint32*)&a_page_directories;
int	paging_pid = 0;

//Physical
//Uint32	superpageBitmap[640];
//Uint32	pageBitmap[20480];
Uint32	*superpageBitmap;	// Physical Page Allocation bitmap
Uint32	*pageBitmap;		//  Allocated in `memmanager_install`
Uint	mm_maxmem = 0;		// Memory installed in bytes
Uint	mm_ReservedPages = 0;	// Reserved page count
Uint	mm_UsedPhysPages = 0;
Uint	mm_TotalPages = 0;		// Total pages

//Virtual (Only for 0xC-0xE (2 Blocks))
Uint32	vsuperpageBitmap[80];
Uint32	vpageBitmap[2560];
// Physical Hardware Mappings (0xF2 - <1 Block)
Uint32	hwSuperpageBitmap[40-8];
Uint32	hwPageBitmap[1280-256];

Uint32	*page_directory[PROC_MAX_PID];	//Virtual
Uint32	page_directoryP[PROC_MAX_PID];	//Physical
Uint32	*giPageTables = (Uint32*)TABLE_ADDR;
Uint32	*giPageDirs = (Uint32*)DIR_ADDR;
Uint16	*giRefCount = (void*)REF_ADDR;

//EXTERNAL FUNCTIONS
extern Uint32	read_cr0();
extern void	write_cr0(Uint32);
extern Uint32	read_cr3();
extern void	write_cr3(Uint32);
extern void MM_SetPid(int pid);
extern void phys_copyPageLow(Uint32 dest, Uint32 src);
extern void Proc_DumpRunning();
extern void Proc_Yield();

// ==== PROTOTYPES ====
void MM_RefPage(Uint32 page);
void MM_DerefPage(Uint32 page);
void MM_PagetableFlush();
void MM_CopyPhys(Uint dest, Uint src);
Uint MM_Duplicate(Uint addr);

//CODE
/**
 \fn void *MM_AllocPhys()
 \brief Allocates a physical page in memory
 
 This function finds a free page in the computer's memory,
 marks it as used and returns a pointer to it.
*/
#define	allocphys	MM_AllocPhys
void *MM_AllocPhys()
{
	int	a,b,c;
	int	spcount = mm_TotalPages >> 10;
	int	pageId;
	void *mem;
	
	if(mm_UsedPhysPages == mm_TotalPages)
		panic("MM_AllocPhys - Out of memory! (1)\n");
	
	//LogF("MM_AllocPhys: ()\n");
	LOCK();
	for(a=0; a < spcount && superpageBitmap[a] == 0xFFFFFFFF; a++);
	//LogF(" MM_AllocPhys: a=%i\n", a);
	if(a == spcount && spcount != 0)	panic("MM_AllocPhys - Out of memory! (2)\n");	// Panic
	for(b=0; superpageBitmap[a] & 1<<b; b++);
	//LogF(" MM_AllocPhys: b=%i\n", b);
	for(c=0; pageBitmap[a*32+b] & 1<<c; c++);
	//LogF(" MM_AllocPhys: c=%i\n", c);
	
	pageBitmap[a*32+b] |= 1<<c;
	if(pageBitmap[a*32+b] == 0xFFFFFFFF)
		superpageBitmap[a] |= 1<<b;
	//LogF(" MM_AllocPhys: a=%i,b=%i,c=%i\n", a, b, c);
	UNLOCK();
	//LogF(" MM_AllocPhys: a=%i,b=%i,c=%i\n", a, b, c);
	
	pageId  = a << 10;
	pageId |= b << 5;
	pageId |= c << 0;
	
	mem = (void *) (pageId << 12);
	
	// Theoretically Impossible
	if((Uint) mem > mm_maxmem)
	{
		//Panic
		panic("==== PANIC: Out of memory! ====");
		for(;;);
	}
	
	#if DEBUG >= 2
	LogF("MM_AllocPhys: mem (return address) = 0x%x\n", (Uint)mem);
	#endif
	
	mm_UsedPhysPages ++;
	
	return mem;
}

/**
 \fn void *MM_AllocPhysCont(int count)
 \brief Allocates several physical pages in memory
 \param count	Number of contiguous pages to allocate
*/
void *MM_AllocPhysCont(int count)
{
	int	a,b,c, i;
	int	spcount = mm_TotalPages >> 10;
	int	pageId=0;
	int	search=1;
	void *mem;
	
	LOCK();
	while(search)
	{
		for(a=0; superpageBitmap[a] == 0xFFFFFFFF && a < spcount; a++);
		if(a == spcount && spcount != 0)	panic("MM_AllocPhysCont - Out of memory!");	// Panic
		for(b=0; superpageBitmap[a] & 1<<b; b++);
		for(c=0; pageBitmap[a*32+b] & 1<<c; c++);
		
		pageId  = a << 10;
		pageId |= b << 5;
		pageId |= c << 0;
		
		search = 0;
		for(i=0;i<count;i++)
		{
			if( pageBitmap[ (pageId+i)>>5 ] & 1 << ((pageId+i)&0x1F) ) {
				search = 1;
				break;
			}
		}
	}
	
	for(i=0;i<count;i++)
	{
		pageBitmap[ (pageId+i)>>5 ] |= 1 << ((pageId+i)&0x1F);
		if(pageBitmap[ (pageId+i)>>5 ] == 0xFFFFFFFF)
			superpageBitmap[ (pageId+i)>>10 ] |= 1 << (( (pageId+i)>>5 )&0x1F);
	}
	UNLOCK();
	
	mem = (void *) (pageId << 12);
	
	#if DEBUG >= 2
	LogF("MM_AllocPhysCont: RETURN 0x%x\n", mem);
	#endif
	
	mm_UsedPhysPages ++;
	
	return mem;
}

/**
 \fn void freephys(void *mem)
 \param mem The address of the memory to free
 \brief Frees a page from memory;
*/
void freephys(void *mem)
{
	Uint	addr;
	int a,b,d;
	
	addr = (Uint) mem;
	
	if(addr & 0xFFF) {
		warning("freephys - Attempt to free memory that is not page aligned\n");
		return;
	}
	
	addr = addr >> 12;	// Get Page ID
	a = addr >> 10;
	b = (addr >> 5) & 0x1F;
	d = addr & 0x1F;
	
	LOCK();
	pageBitmap[a*32+b] &= ~(1<<d);
	if(pageBitmap[a*32+b] == 0)
		superpageBitmap[a] &= ~(1<<b);
	UNLOCK();
		
	mm_UsedPhysPages --;
}

/**
 \fn void MM_RefPage(Uint32 pageAddr)
 \brief Increases the reference count on a page
*/
void MM_RefPage(Uint32 pageAddr)
{
	pageAddr >>= 12;
	giRefCount[pageAddr] ++;
}
/**
 \fn void MM_DerefPage(Uint32 pageAddr)
 \brief Decreases the reference count on a page
*/
void MM_DerefPage(Uint32 pageAddr)
{
	pageAddr >>= 12;
	if(giRefCount[pageAddr] > 0)
		giRefCount[pageAddr] --;
}

/**
 \fn void memmanager_install(Uint32 memory)
 \brief Initalizes the Memory Manager
*/
void memmanager_install(Uint32 memory)
{
	int a;
	Uint32	*dirK = (Uint32*)&page_dir_kernel;
	Uint32	*tabR = (Uint32*)&page_table_root;
	Uint32	*tabS = (Uint32*)&page_table_stack;
	Uint32	*tabK = (Uint32*)&page_table_kernel;
	
	// Set Max Memory Value
	mm_maxmem = memory;		//Low Memory is 1Mb
	
	LogF("[MM  ] Building Page Bitmaps... ");
	// --- Build Allocation Bitmaps
	{
		int	count;
		mm_TotalPages = count = memory >> 12;	// Page Count (Saved to global)
		count = count / 32;	// DWord Count (Size of page bitmap)
		count = count / 8;	// Super Page byte count
		count = (count+3) & 0xFFFFFFC;	// DWord Align
		superpageBitmap = &gKernelEnd + 0xC0000000 + 24*4096/4;	// Kernel End (Virtual) plus stack space
		pageBitmap = superpageBitmap + count/4;
		// Clear Physical Bitmaps
		memsetda(superpageBitmap, 0, count/4+count*2);
		// Clear Virtual Bitmap
		memsetda(vsuperpageBitmap, 0, sizeof(vsuperpageBitmap)/sizeof(vsuperpageBitmap[0]));
		memsetda(vpageBitmap, 0, sizeof(vpageBitmap)/sizeof(vpageBitmap[0]));
		// Clear Hardware Bitmap
		memsetda(hwSuperpageBitmap, 0, sizeof(hwSuperpageBitmap)/sizeof(hwSuperpageBitmap[0]));
		memsetda(hwPageBitmap, 0, sizeof(hwPageBitmap)/sizeof(hwPageBitmap[0]));
	}
	// --- END
	// --- Mark Used Pages
	{
		int end;
		end = (Uint) &gKernelEnd;
		end += memory >> (12+3);	// Page Bitmap
		end += memory >> (12+5+3);	// Superpage Bitmap
		end = (end+0xFFF) >> 12;	// Round to page count
		end += 24;		// PID0 Stack Space (24 Pages)
		// Rows of 32 pages allocated
		for( a = 0; a < (end >> 5); a++ )	pageBitmap[a] = -1;
		// Extra pages
		for( a = 0; a < (end & 0x1F); a++ )	pageBitmap[(end>>5)] |= 1 << a;
		// 32 Superpages
		for( a = 0; a < (end >> 10); a++ )	superpageBitmap[a] = -1;
		// Single Superpages
		for( a = 0; a < ((end >> 5)&0x1F); a++ )	superpageBitmap[ (end >> 10) ] |= 1 << a;
		// Save unusable end
		mm_ReservedPages = end;
		mm_UsedPhysPages = end;
	}
	// --- END
	LogF("Done.\n");
	
	// Clear Page Directories
	memsetda(page_directory, 0, PROC_MAX_PID);
	
	LogF("[MM  ] Filling Kernel Page Tables... ");
	// --- Build kernel page tables
	for(a=0;a<1024;a++)	tabR[a] = a*4096+3;
	// Kernel Heap Memory (allocated in heap initialization) at 0xC0400000
	for(a=0;a<1024;a++)	tabK[a] = 0;
	
	// Clear Kernel Directory
	for(a=0;a<KERNEL_DIR;a++)	dirK[a] = 0;
	
	// --- Build Essential Kernel Structures
	//  - Code, Heap and Stack
	dirK[KERNEL_DIR] = ((Uint)tabR - KERNEL_BASE) | 3;	//0xC0000000
	dirK[KERNEL_DIR+1] = ((Uint)tabK - KERNEL_BASE) | 3;	//0xC0400000
	dirK[KSTACK_DIR] = ((Uint)tabS - KERNEL_BASE) | 3;	//0xEFC00000
	//  - Fractal Mappings
	dirK[TABLE_DIR] = ((Uint)dirK - KERNEL_BASE) | 3;		//0xF0000000
	dirK[DIR_DIR] = ((Uint)&a_page_directories - KERNEL_BASE) | 3;
	// --- END
	
	// Set PID#0 Page Directory
	page_directories[0] = ((Uint32) dirK - KERNEL_BASE) | 3;
	
	// --- Allocate persistent page tables
	
	//  - Kernel Code, Handles (Ignore Heap, It is allocated in `start.asm`)
	for( a = KERNEL_DIR+2; a < KSTACK_DIR; a++ )	dirK[ a ] = ((Uint)MM_AllocPhys()) | 3;
	memsetda( TABLEPTR(KERNEL_DIR+2, 0), 0, 1024 * (KSTACK_DIR-KERNEL_DIR-2) );
	
	//  - Memory Mapped Data & Temp Pages
	for( a = MEMMAP_DIR; a < 1024; a++ )	dirK[ a ] = ((Uint)MM_AllocPhys()) | 3;
	memsetda( TABLEPTR(MEMMAP_DIR, 0), 0, 1024 * (1024-MEMMAP_DIR) );
	
	//  - Usermode Code
	dirK[ 1023 ] |= 4;	// Set User
	TABLEA(USERMODE_ADDR) = ((Uint)&gUserCode - KERNEL_BASE) | 5;	//User,RO,Present
	// --- END
	LogF("Done.\n");
	
	LogF("[MM  ] Building & Filling Reference Tables... ");
	// --- Build Reference Tables
	memsetda( TABLEPTR(REF_DIR, 0), 0, 1024 );	// Clear Memory
	a = mm_TotalPages;	// Total Pages Count
	a *= sizeof(*giRefCount);	// Total Size of Buffer
	a = (a + 0xFFF) >> 12;	// Pages to Make Buffer (Round to page)
	
	for(;a--;)	TABLE(REF_DIR, a+REF_PAGE) = (Uint)MM_AllocPhys() | 3;
	
	a = mm_TotalPages;	// Total Pages Count
	a *= sizeof(*giRefCount);	// Total Size of Buffer
	memsetda( giRefCount, 0, a/4 );	
	// --- END
	
	// --- Mark References
	memsetwa(giRefCount, 1, mm_UsedPhysPages);
	// --- END

	LogF("Done.\n");
	
	LogF(
		"[MM  ] Total Pages: %i (%iMb), %i:%i (%iMiB:%iMiB) Used:Unused\n",
		mm_TotalPages, mm_TotalPages >> (20-12),
		mm_UsedPhysPages,
		mm_TotalPages - mm_UsedPhysPages,
		mm_UsedPhysPages >> (20-12),
		(mm_TotalPages - mm_UsedPhysPages) >> (20-12)
		);
	
	// Fomally Set Paging ID
	paging_setpid(0);
}

void paging_enable()
{
	write_cr0(read_cr0() | 0x80000000);
}
void paging_disable()
{
	write_cr0(read_cr0() & ~0x80000000);
}

void paging_setpid(int pid)
{
	#if 1
	MM_SetPid(pid);
	#else
	Uint	paddr;
	
	#if DEBUG
		puts("Changing PID\n");
	#endif
	paddr = ((Uint)page_directories[pid]) & 0xFFFFF000;
	if(paddr == 0) {
		panic("Changing to an empty address space");
	}
	//#if DEBUG
	LogF(" New CR3 = 0x%x\n", paddr);
	//#endif
	paging_pid = pid;
	write_cr3(paddr);
	#endif
}

void MM_PagetableFlush()
{
	write_cr3( ((Uint32)page_directories[paging_pid]) & 0xFFFFF000 );
}

/**
 \fn int MM_CloneKernel()
 \brief Clones an address space from only >0xC
 \return ID of new address space
*/
int MM_CloneKernel()
{
	int pid;
	int	i, j;
	Uint	paddr;
	
	// ---- Get new ID ----
	for( pid = 0; page_directories[pid] && pid < MAX_DIRS; pid++ );
	if(pid == MAX_DIRS) {
		warning("MM_CloneKernel - No space for page directory\n", pid);
		return -1;
	}
	
	// Allocate New Page Directory
	page_directories[pid] = ((Uint)allocphys()) | FLAG_WRITEABLE | FLAG_PRESENT;
	MM_RefPage(page_directories[pid]);
	MM_PagetableFlush();
	
	// Copy page directories
	memsetda( DIRPTR(pid, 0), 0, 0x300 );
	memcpyda( DIRPTR(pid, 0x300), DIRPTR(paging_pid, 0x300), 0x100 );
	
	// Map new PD
	DIR(pid, TABLE_DIR)	= page_directories[pid];
	DIR(paging_pid, TABLE_T_DIR) = page_directories[pid];
	
	// Copy Local Storage
	for( i = PPDATA_DIR; i < PPDATA_DIR + 64; i++ )	// Data Range (256 Mb)
	{
		if( !(DIR(pid, i) & FLAG_PRESENT) )	continue;
		
		//Get and fill new page
		paddr = MM_Duplicate( DIR(pid, i) & ~0xFFF);
		MM_RefPage(paddr);
		
		//Set directory entry
		DIR(pid, i) = paddr | 3;
	}
	for( i = PPDATA_DIR; i < PPDATA_DIR + 64; i++ )	// Data Range (256 Mb)
	{
		if( !(DIR(pid, i) & FLAG_PRESENT) )	continue;
		for( j = 0; j < 1024; j++ )
		{
			// Check if used
			if( !(TABLE(i,j) & FLAG_PRESENT) )	continue;
			// Copy
			paddr = MM_Duplicate( TABLE(i, j) & ~0xFFF );
			MM_RefPage(paddr);
			TABLETMP(i, j) = paddr | 3;
		}
	}
	// Clean Up
	DIR(paging_pid, TABLE_T_DIR) = 0;
	
	return pid;
}

/**
 \fn int MM_CloneCurrent(void)
 \brief Clones the current address space to another
 \return ID of new address space
*/
int MM_CloneCurrent()
{
	int i, j;
	Uint32	paddr;
	int	pid;
	
	// ---- Get new ID ----
	for( pid = 0; page_directories[pid] && pid < MAX_DIRS; pid++ );
	if(pid == MAX_DIRS)
	{
		warning("MM_CloneCurrent - No space for page directory\n", pid);
		return -1;
	}
	
	#if DEBUG
	LogF("Set COW for original\n");
	#endif
	
	// ---- Set Copy-On-Write for old table ----
	for(i=KERNEL_DIR;i--;)
	{
		if( !(DIR(paging_pid, i) & FLAG_PRESENT) )
			continue;
		
		// Set Copy on write to individual pages
		for(j=1024;j--;)
		{
			if( !(TABLE(i, j) & FLAG_PRESENT) )
				continue;
			//Set Attributes
			if(TABLE(i, j) & FLAG_WRITEABLE) {
				TABLE(i, j) |= FLAG_CPYONWRITE;
				TABLE(i, j) &= ~FLAG_WRITEABLE;
			}
		}
	}
	
	#if DEBUG
	LogF("Alloc New PD\n");
	#endif
	// Allocate Page Directory
	page_directories[pid] = ((Uint)allocphys()) | FLAG_WRITEABLE | FLAG_PRESENT;
	MM_RefPage(page_directories[pid]);

	// Copy Page Directory
	memcpyda( DIRPTR(pid, 0), DIRPTR(paging_pid, 0), 0x1000/4 );
	// Set Fractal Map on new Dir
	DIR(pid, TABLE_DIR) = page_directories[pid];
	// Map in new Dir. for editing
	DIR(paging_pid, TABLE_T_DIR) = page_directories[pid];
	MM_PagetableFlush();
	
	#if DEBUG
	LogF("Copy New User Data\n");
	#endif
	// ---- Copy Directory Entries ----
	for(i=KERNEL_DIR;i--;)
	{
		if( !(DIR(pid, i) & FLAG_PRESENT) )
			continue;
		
		//Get and fill new page
		//paddr = (Uint32)allocphys();
		//MM_RefPage(paddr);
		//MM_CopyPhys(paddr, DIR(pid, i) & 0xFFFFF000);
		paddr = MM_Duplicate( DIR(pid, i) & ~0xFFF );
		MM_RefPage(paddr);
		
		//Set directory entry
		DIR(pid, i) &= ~0xFFFFF000;
		DIR(pid, i) |= paddr;
		
		// Set Copy on write to individual pages
		for(j=1024;j--;)
		{
			if( !(TABLETMP(i, j) & FLAG_PRESENT) )
				continue;
			MM_RefPage(TABLETMP(i, j));
		}
	}

	#if DEBUG
	LogF("Copy Local Storage (Dirs)\n");
	#endif
	// ---- Copy Local Storage ----
	for( i = PPDATA_DIR; i < PPDATA_DIR + 64; i++ )	// Data Range (256 Mb)
	{
		if( !(DIR(pid, i) & FLAG_PRESENT) )
			continue;
		
		//Get and fill new page
		paddr = (Uint)allocphys();
		MM_RefPage(paddr);
		//MM_CopyPhys(paddr, DIR(pid, i) & 0xFFFFF000);
		
		//Set directory entry
		DIR(pid, i) = paddr | 3;
		
		// Flush Page Entry
		INVLPG( TABLETMPPTR(i, 0) );
		// Clear
		memsetda(TABLETMPPTR(i, 0), 0, 1024);
	}
	
	#if DEBUG
	LogF("Flushing CR3\n");
	#endif
	MM_PagetableFlush();
	
	#if DEBUG
	LogF("Copy Local Storage (Tables)\n");
	#endif
	for( i = PPDATA_DIR; i < PPDATA_DIR + 64; i++ )	// Data Range (256 Mb)
	{
		if( !(DIR(pid, i) & FLAG_PRESENT) )
			continue;
		// Copy Pages
		for( j = 0; j < 1024; j++ )
		{
			// Check if used
			if( !(TABLE(i,j) & FLAG_PRESENT) )
				continue;
			
			// Copy
			//paddr = (Uint)allocphys();
			//MM_RefPage(paddr);
			//MM_CopyPhys( paddr, TABLE(i, j) & 0xFFFFF000 );
			paddr = MM_Duplicate( TABLE(i, j) & ~0xFFF );
			MM_RefPage(paddr);
			TABLETMP(i, j) = paddr | 3;
		}
	}
	
	#if DEBUG
	LogF("Cleaning Up\n");
	#endif
	// Unset Temp Table
	DIR(paging_pid, TABLE_T_DIR) = 0;
	
	return pid;
}

Uint MM_Duplicate(Uint addr)
{
	Uint	dest, tmp, tmp2;
	dest = (Uint)MM_AllocPhys();
	tmp = MM_MapTemp(dest);
	tmp2 = MM_MapTemp(addr);
	memcpyda((void*)tmp, (void*)tmp2, 1024);
	MM_FreeTemp(tmp);
	MM_FreeTemp(tmp2);
	return dest;
}

/**
 \fn void MM_Dump()
 \brief Dump the Memory Space
*/
void MM_Dump()
{
	int i, dir, tab;
	Uint	vStart = -1;
	Uint	pStart = 0;
	Uint	pNext = 0;
	
	LogF("Virtual Memory Map for Paging PID #%i\n", paging_pid);
	LogF(" CR3 = 0x%08x\n", page_directories[paging_pid]);
	
	for(i=0;i<1024*1024;i++)
	{
		dir = i >> 10;
		tab = i & 0x3FF;
		
		if( !(DIR(paging_pid, dir) & FLAG_PRESENT) )
			i += 1023;
		if( !(DIR(paging_pid, dir) & FLAG_PRESENT) || !(TABLE(dir, tab) & FLAG_PRESENT) )
		{
			if(vStart != -1)
			{
				LogF(" 0x%08x-0x%08x => 0x%08x-0x%08x\n", vStart, ((dir<<22)|(tab<<12))-1, pStart, pNext-1);
				vStart = -1;
			}
			continue;
		}
		
		//LogF("0x%08x => 0x%08x\n", (dir<<22)|(tab<<12), TABLE(dir, tab));
		
		if(vStart != -1)
		{
			if((TABLE(dir,tab) & 0xFFFFF000) != pNext)
			{
				LogF(" 0x%08x-0x%08x => 0x%08x-0x%08x\n", vStart, ((dir<<22)|(tab<<12))-1, pStart, pNext-1);
				vStart = (dir<<22)|(tab<<12);
				pStart = TABLE(dir,tab) & 0xFFFFF000;
				pNext = pStart + 0x1000;
			} else {
				pNext += 0x1000;
			}
		} else {
			vStart = (dir<<22)|(tab<<12);
			pStart = TABLE(dir,tab) & 0xFFFFF000;
			pNext = pStart + 0x1000;
		}
	}
}

/**
 \fn void MM_Clear()
 \brief Clears the current address space
*/
void MM_Clear()
{
	int a,b;
	
	// Check for process memory
	if( page_directories[paging_pid] == 0 ) {
		warning("MM_Clear - Process #%i does not have memory\n", paging_pid);
		return;
	}
	
	#if DEBUG
		LogF("MM_Clear: ()\n");
	#endif
	
	// Loop all user directories
	for( a = KERNEL_DIR; a--; )
	{
		// Check if used
		if( DIR(paging_pid, a) == 0 )
			continue;
		
		// Loop pages
		for(b=1024;b--;)
		{
			// Check if used
			if( !(TABLE(a,b) & FLAG_PRESENT) )
				continue;
			
			// Dereference and free if unused
			MM_DerefPage(TABLE(a,b)&0xFFFFF000);
			if(giRefCount[TABLE(a,b)>>12] == 0)
				freephys( (void*)(TABLE(a,b)&0xFFFFF000) );
			
			TABLE(a,b) = 0;
		}
		
		// Dereference directory and free if unused
		MM_DerefPage( DIR(paging_pid, a) & 0xFFFFF000 );
		if(giRefCount[ DIR(paging_pid, a) >> 12 ] == 0)
			freephys( (void*)(DIR(paging_pid, a) & 0xFFFFF000) );
		
		DIR(paging_pid, a) = 0;
	}
	#if DEBUG
		LogF("MM_Clear: User Data Cleared\n");
	#endif
}

/**
 \fn void MM_MapEx(Uint vaddr, Uint ppage, int flags)
 \brief Maps a physical page to a virtual with supplied flags
 \param vaddr	Address - Virtual Address
 \param ppage Address - Physical Page address
 \param flags	Integer - Flags to apply to mapping
*/
int MM_MapEx(Uint vaddr, Uint ppage, int flags)
{	
	if( !(DIR(paging_pid, KERNEL_DIR) & 1) )
		panic("MM_MapEx - Process has no address space. WTF?\n");
	
	// Sanitize Arguments
	ppage = ppage & ~0xFFF;
	
	// Create Directory if it does not exist
	if( DIRA(vaddr) == 0 ) {
		DIRA(vaddr) = ((Uint)allocphys()) | 7;	//111 - User,RW,Present
		MM_RefPage( DIRA(vaddr) );
		memsetda( TABLEPTR(vaddr>>22, 0), 0, 0x1000/4 );
	}
	// Check if memory is already mapped
	else if( TABLEA(vaddr) != 0 ) {
		warning("MM_MapEx - Memory alread mapped at this address (0x%x)\n", vaddr);
		return 0;
	}
	
	// Map
	TABLEA(vaddr) = ppage | (flags & 0xC06) | 1;
	MM_RefPage( ppage );
	return 1;
}

/**
 \fn void mm_map(Uint vaddr, Uint page)
 \brief Maps a physical page to a virtual one in the current address space
 \param vaddr	Address - Virutal Address to map to
 \param page	Pointer - Physical Page to map
*/
void
mm_map(Uint vaddr, Uint page)
{	
	//LogF("mm_map: (vaddr=0x%x, page=0x%x)\n", vaddr, page);
	
	if( (Uint32)page_directories[paging_pid] == 0 ) {
		warning("mm_map - Paging PID #%i is not initialised.", paging_pid);
		return;
	}
	
	page = page & ~0xFFF;
	
	if(DIRA(vaddr) == 0) {
		DIRA(vaddr) = ((Uint)allocphys()) | 7;	//111 - User,RW,Present
		MM_RefPage( DIRA(vaddr) );
		memsetda(TABLEPTR(vaddr >> 22, 0), 0, 1024);
	}
	else if(TABLEA(vaddr) & 1) {
		warning("mm_map - Memory already mapped at 0x%08x.\n", vaddr);
		return;
	}
	
	TABLEA(vaddr) = page | 7;	//111 - User,RW,Present
	MM_RefPage(page);
}

/**
 \fn void mm_setro(Uint32 vaddr, int ro)
 \brief Alters the Read-Only Flag
 \param vaddr	Address - Virutal Address to map to
 \param ro	Boolean - Mark page as read only?
*/
void
mm_setro(Uint32 vaddr, int ro)
{	
	if((Uint32)page_directories[paging_pid] == 0) {
		LogF("ERROR: mm_setro - Process #%i has no memory space\n", paging_pid);
		return;
	}
	
	if(DIRA(vaddr) == 0) {
		puts("ERROR: mm_setro - Invalid Page");
		return;
	}
	
	if(ro)	TABLEA(vaddr) |= 2;
	else	TABLEA(vaddr) &= ~2;
}

/**
 \fn Uint32 mm_alloc(Uint32 vaddr)
 \brief Allocates Memory to a specified virtual page
 \param vaddr	Virual Address to allocate
 \return Physical Address Allocated
*/
Uint32 mm_alloc(Uint32 vaddr)
{
	Uint32	paddr;
	
	if((Uint)page_directories[paging_pid] == 0) {
		warning("mm_alloc: Process #%i has no memory space\n", paging_pid);
		return 0;
	}

	// Check if the Directory is mapped
	if( !(DIRA(vaddr) & 1) )
	{
	//	LogF(" mm_alloc: New DIR\n");
		DIRA(vaddr) = ((Uint)allocphys()) | 3;	//111 - User,RW,Present
		MM_RefPage( DIRA(vaddr) );
		memsetda( TABLEPTR(vaddr>>22, 0), 0, 1024 );
	}
	else if( TABLEA(vaddr) != 0)
	{
		warning("mm_alloc - Memory already allocated at 0x%x\n", vaddr&~0xFFF);
		return TABLEA(vaddr) & ~0xFFF;
	}
	
	// Allocate New Page
	paddr = (Uint) allocphys();
	MM_RefPage( paddr );
	// Map on Table
	TABLEA(vaddr) = paddr | 3;
	
	if(vaddr < 0xC0000000) {
		TABLEA(vaddr) |= FLAG_USERMODE;
		DIRA(vaddr) |= FLAG_USERMODE;
	}
	__asm__ __volatile__ ("invlpg (%0)" : : "r" (vaddr) );
	
	//LogF("mm_alloc: RETURN 0x%x\n", paddr);
	return paddr;
}

/**
 \fn void mm_dealloc(Uint32 vaddr)
 \brief De-allocates Memory to a specified virtual page
 \param vaddr	Address - Virual Address
*/
void
mm_dealloc(Uint32 vaddr)
{	
	freephys((Uint8 *)(TABLEA(vaddr) & ~0xFFF));
	MM_DerefPage(TABLEA(vaddr) & ~0xFFF);
	TABLEA(vaddr) = 0;
}

void
MM_SetKernel(Uint32 vaddr)
{	
	if( !(DIRA(vaddr) & FLAG_PRESENT) )
		return;
	if( !(TABLEA(vaddr) & FLAG_PRESENT) )
		return;
	TABLEA(vaddr) &= ~FLAG_USERMODE;
}

void
mm_remove(int pid)
{	
	int a, b;
	
	// Check for process memory
	if( page_directories[pid] == 0 ) {
		warning("mm_remove - Process #%i does not have memory\n", paging_pid);
		return;
	}
	
	// Fractal map to temp
	DIRA(TABLE_T_ADDR) = page_directories[pid];
	
	// Loop all user directories
	for( a = KERNEL_DIR; a--; )
	{
		// Check if used
		if( !(DIR(pid, a) & FLAG_PRESENT) )
			continue;
		
		// Loop pages
		for(b=1024;b--;)
		{
			// Check if used
			if( !(TABLETMP(a,b) & FLAG_PRESENT) )
				continue;
			
			// Dereference and free if unused
			MM_DerefPage( TABLETMP(a,b)&0xFFFFF000 );
			if(giRefCount[TABLETMP(a,b)>>12] == 0)
				freephys( (void*)(TABLETMP(a,b)&0xFFFFF000) );
		}
		
		// Dereference directory and free if unused
		MM_DerefPage( DIR(pid, a) & 0xFFFFF000 );
		if(giRefCount[ DIR(pid, a) >> 12 ] == 0)
			freephys( (void*)(DIR(pid, a) & 0xFFFFF000) );
	}
	
	DIR(paging_pid, TABLE_T_DIR) = 0;
	
	// Dereference, Free and clear directory
	MM_DerefPage( page_directories[pid]&0xFFFFF000 );
	if( giRefCount[ page_directories[pid] >> 12 ] == 0)
		freephys( (Uint32*)(page_directories[pid]&0xFFFFF000) );
	page_directories[pid] = 0;
}

/**
 \fn void MM_PageFault(Uint addr, t_regs *r)
 \brief Page Fault Handler
*/
void MM_PageFault(Uint addr, t_regs *r)
{
	int page, dir;
	Uint	paddr;
	Uint	eip = r->eip;
	Uint	errno = r->err_code;
	
	page = addr >> 12;
	dir = page >> 10;
	page &= 0x1FF;
	
	// Write When Not Allowed
	if( DIRA(addr) & FLAG_PRESENT && TABLEA(addr) & FLAG_PRESENT
	&& !(TABLEA(addr) & FLAG_WRITEABLE) )
	{
		if( TABLEA(addr) & FLAG_CPYONWRITE )
		{
			//LogF(" MM_PageFault: PID%i - Copy-On-Write, ", proc_pid);
			if(giRefCount[ TABLEA(addr) >> 12 ] > 1)
			{
			//	LogF("New Page\n");
				//Get and fill new page
				paddr = MM_Duplicate( TABLEA(addr) & ~0xFFF );
				MM_RefPage(paddr);
				MM_DerefPage( TABLEA(addr) );
				TABLEA(addr) = paddr | 7;	// User,RW,Present
			}
			//else
			//	LogF("Use old page\n");
			TABLEA(addr) |= FLAG_WRITEABLE;
			TABLEA(addr) &= ~FLAG_CPYONWRITE;
			return;
		}
	}
	
	puts("Addr: ");	putHex(addr);	puts(" EIP: ");	putHex(eip);	puts("\n");
	warning("Page Fault - Code at 0x%x referenced 0x%x (errno=0x%x)\n", eip, addr, errno);
	//LogF("MM_PageFault: (addr=0x%x, eip=0x%x, errno=0x%x)\n", addr, eip, errno);
	//LogF(" Page Present:%B, Write:%B, User Mode:%B, Reserved Bits Trashed:%B, Instruction Fetch:%B\n",
	//	errno&1, errno&2, errno&4, errno&8, errno&16);
	
	if(errno & 8)
		LogF("Reserved Bits Trashed!\n");
	else
	{
		if(errno & 4)
			LogF("User ");
		else
			LogF("Kernel ");
		if(errno & 2)
			LogF("write to ");
		else
			LogF("read from ");
		if(errno & 1)
			LogF("bad/locked ");
		else
			LogF("non-present ");
		LogF("memory");
		if(errno & 16)
			LogF(" (Instruction Fetch)");
		if(errno & 8)
			LogF(" Reserved bits were trashed.");
		LogF("\n");
	}
	
	Debug_Backtrace(eip, r->ebp);
	
	if( !(errno & 0x10) && eip != addr )	Debug_Disasm(eip);
	
	// User & Locked Memory - Kill Proc
	if(errno & 1 && errno & 4)
	{
		warning("Illegal Memory Access (Address 0x%x) in PID#%i\n"
			"EAX: %08x\tEBX: %08x\tECX: %08x\tEDX: %08x\n"
			"ESI: %08x\tEDI: %08x\tEBP: %08x\tESP: %08x\n",
			addr, proc_pid,
			r->eax, r->ebx,	r->ecx, r->edx,
			r->esi, r->edi,	r->ebp, r->esp
			);
		goto lUserExit;
	}
	
	// Directory Not Present
	if( !(DIRA( addr ) & FLAG_PRESENT)
	|| (!(TABLEA( addr ) & FLAG_PRESENT) && TABLEA( addr ) == 0) )
	{
		if(errno & 4)	//Is User
		{
			warning("Illegal Memory Access (Address 0x%x) in PID#%i (Dir not present)\n"
				"EAX: %08x\tEBX: %08x\tECX: %08x\tEDX: %08x\n"
				"ESI: %08x\tEDI: %08x\tEBP: %08x\tESP: %08x\n",
				addr, proc_pid,
				r->eax, r->ebx,	r->ecx, r->edx,
				r->esi, r->edi,	r->ebp, r->esp
				);
			goto lUserExit;
		}
		//#if DUMP_ON_PF
			MM_Dump();
		//#endif
		panic(
			"Illegal Memory Access (Address 0x%x) by Kernel in PID#%i (Dir not present)\n"
			"EAX: %08x\tEBX: %08x\tECX: %08x\tEDX: %08x\n"
			"ESI: %08x\tEDI: %08x\tEBP: %08x\tESP: %08x\n",
			addr, proc_pid,
			r->eax, r->ebx,	r->ecx, r->edx,
			r->esi, r->edi,	r->ebp, r->esp
			);
		return;
	}
	
	if( !(TABLEA(addr) & FLAG_PRESENT) )
	{
		warning("UNIMPLEMENTED: Disk Paging\n");
		return;
	}
	
	// Write When Not Allowed
	if( !(TABLEA(addr) & FLAG_WRITEABLE) )
	{
		warning("Illegal Memory Access (Address 0x%x) in PID#%i (Read-Only)\n", addr, proc_pid);
		goto lUserExit;
	}
	warning("Undefined Paging Error (Address 0x%x) in PID#%i\n", addr, proc_pid);
	return;
	
lUserExit:
	Proc_DumpRunning();
	Proc_Exit();
	Proc_Yield();
}

/*
 int MM_IsValid(Uint addr)
Checks if an address is a valid paged address
*/
int MM_IsValid(Uint addr)
{	
	if(!(DIRA( addr ) & 1))		return 0;
	if(!(TABLEA( addr ) & 1))	return 0;
	
	return 1;
}

int	MM_IsVPageAlloc(int page)
{
	if( vpageBitmap[ page>>5 ] & 1<<(page&0x1F) )
		return 1;
	return 0;
}

/*
void* MM_AllocRange(int rangeMsn, int count)
- Allocates a specified number of pages in the range supplied
- Used to load modules to 0xD0000000 Range
*/
void*
MM_AllocRange(int rangeMsb, int count)
{
	int start;
	int	a,b,c;
	int	i, search = 1;
	int	pageId;
	void *mem;
	
	
	//#if DEBUG
		LogF("MM_AllocRange: (rangeMsb=0x%02x, count=%i)\n", rangeMsb, count);
	//#endif
	
	start = rangeMsb - 0xC0;
	start <<= 2;	// Super Page Index
	
	// Find a free range
	while(search)
	{
		search = 0;
		for(a=start; vsuperpageBitmap[a] == 0xFFFFFFFF && a < 80; a++);
		if(a == 80) {
			//Panic
			warning("MM_AllocRange - Out of space in 0x%x range\n", rangeMsb);
			return NULL;
		}
		for(b=0; vsuperpageBitmap[a] & 1<<b; b++);
		for(c=0; vpageBitmap[a*32+b] & 1<<c; c++);
		
		//Check for space further on
		pageId = (a * 32 + b) * 32 + c;
		for(i=1;i<count;i++)
		{
			pageId++;
			if( MM_IsVPageAlloc(pageId) )
			{
				search = 1;
				break;
			}
		}
	}
	// Build Page ID
	a += 0xC0 << 2;
	pageId  = a << 10;
	pageId |= b << 5;
	pageId |= c << 0;

	//#if DEBUG
		LogF(" MM_AllocRange: pageId = 0x%x\n", pageId);
	//#endif
	
	mem = (void *) (pageId << 12);
	
	// Allocate Pages
	for(i=0;i<count;i++)
	{
		mm_alloc( (pageId+i)<<12 );	//Allocate
		MM_SetKernel( (pageId+i)<<12 );	// Set as kernel page
		vpageBitmap[ (pageId+i)>>5 ] |= 1 << ((pageId+i)&0x1F);
		if(vpageBitmap[ (pageId+i)>>5 ] == 0xFFFFFFFF)
			vsuperpageBitmap[ (pageId+i)>>10 ] |= 1 << (((pageId+i)>>5)&0x1F);
	}
	
	//#if DEBUG
		LogF(" MM_AllocRange: RETURN 0x%x\n", (Uint)mem);
	//#endif
	
	return mem;
}

void MM_DeallocRange(void *ptr, int count)
{
	int pageId = (Uint)ptr >> 12;
	
	for(;count--;pageId++)
	{
		mm_dealloc( pageId<<12 );	//Allocate
		vpageBitmap[ pageId>>5 ] &= ~(1 << (pageId&0x1F));
		if(vpageBitmap[ pageId>>5 ] == 0)
			vsuperpageBitmap[ pageId>>10 ] &= ~(1 << ((pageId>>5)&0x1F));
	}
}

int	MM_IsHWPageAlloc(int page)
{
	if( hwPageBitmap[ page>>5 ] & 1<<(page&0x1F) )
		return 1;
	return 0;
}

/**
 \fn void MM_UnmapHW(void *ptr, int count)
 \brief Unmaps a physical range from a virtual address
*/
void MM_UnmapHW(void *ptr, int count)
{
	int pageId = (Uint)ptr >> 12;
	
	if((Uint)ptr < 0xF2000000)	return;
	
	for(;count--;pageId++)
	{
		TABLE( pageId>>10, pageId&0x3FF ) = 0;	//Deallocate
		hwPageBitmap[ pageId>>5 ] &= ~(1 << (pageId&0x1F));
		if(hwPageBitmap[ pageId>>5 ] == 0)
			hwSuperpageBitmap[ pageId>>10 ] &= ~(1 << ((pageId>>5)&0x1F));
	}
}

/**
 \fn void *MM_MapHW(Uint phys, int count)
 \brief Map a physical range and return the virtual address
*/
void *MM_MapHW(Uint phys, int count)
{
	int start = 0;
	int	a,b,c;
	int	i, search = 1;
	int	bPageId, pageId;
	void *mem;
	
	#if DEBUG
		LogF("MM_MapHW: (phys=0x%x, count=%i)\n", phys, count);
	#endif
	
	// --- Find a free range
	while(search)
	{
		search = 0;
		for(a=start; hwSuperpageBitmap[a] == 0xFFFFFFFF && a < 32; a++);
		if(a == 32) {
			//Panic
			warning("MM_MapHW - Out of space in 0xF2-> range\n");
			return NULL;
		}
		for(b=0; hwSuperpageBitmap[a] & 1<<b; b++);
		for(c=0; hwPageBitmap[a*32+b] & 1<<c; c++);
		
		//Check for space further on
		pageId = (a * 32 + b) * 32 + c;
		for(i=1;i<count;i++)
		{
			pageId++;
			if( MM_IsHWPageAlloc(pageId) )
			{
				search = 1;
				break;
			}
		}
	}
	// Build Page ID
	bPageId  = a << 10;
	bPageId |= b << 5;
	bPageId |= c << 0;
	a += 0xF2 << 2;
	pageId  = a << 10;
	pageId |= b << 5;
	pageId |= c << 0;

	#if DEBUG
		LogF(" MM_MapHW: pageId = 0x%x\n", pageId);
	#endif
	
	mem = (void *) (pageId << 12);
	
	// Allocate Pages
	for(i=0;i<count;i++)
	{
		a = (pageId+i) >> 10;
		b = (pageId+i) & 0x3FF;
		
		TABLE(a, b) = (phys + (i<<12)) | 3;	//Allocate
		
		hwPageBitmap[ (bPageId+i)>>5 ] |= 1 << ((bPageId+i)&0x1F);
		if(hwPageBitmap[ (bPageId+i)>>5 ] == 0xFFFFFFFF)
			hwSuperpageBitmap[ (bPageId+i)>>10 ] |= 1 << (((bPageId+i)>>5)&0x1F);
	}
	
	#if DEBUG
		LogF("MM_MapHW: RETURN 0x%x\n", (Uint)mem);
	#endif
	
	return mem;
}

/**
 \fn void *MM_AllocDMA(int pages, Uint *paddr)
 \brief Allocates a contigous region of physical memory and returns it
*/
void *MM_AllocDMA(int pages, Uint *paddr)
{
	Uint ret;
	*paddr = (Uint)MM_AllocPhysCont(pages);
	ret = (Uint)MM_MapHW(*paddr, pages);
	if(ret == 0)
	{
		for(ret=0;ret<pages;ret++)
			freephys( (void*)(*paddr+ret*4096) );
		return NULL;
	}
	return (void*)ret;
}

/**
 \fn void MM_DumpUsage()
*/
void MM_DumpUsage()
{
	LogF("Memory Usage:\n");
	LogF(" %i/%i Pages Used - (%iMb/%iMb)\n",
		mm_UsedPhysPages, mm_TotalPages, mm_UsedPhysPages/256, mm_TotalPages/256);
}

/**
 \fn Uint MM_GetPhysAddr(Uint vaddr)
*/
Uint MM_GetPhysAddr(Uint vaddr)
{
	if(!(DIRA(vaddr) & 1))	return 0;	// Check the Directory
	if(!(TABLEA(vaddr) & 1))	return 0;	// Check the Table
	return (TABLEA(vaddr)&~0xFFF) | (vaddr&0xFFF);
}

/**
 \fn Uint MM_MapTemp(Uint paddr)
 \brief Temporarily maps a physical address into the virtual space
 \param paddr	Physical Address
*/
Uint MM_MapTemp(Uint paddr)
{
	 int	i, j=0;
	//LogF("MM_MapTemp: (paddr=0x%x)\n", paddr);
	for(j=10;j--;)	// Loop until a page is free
	{
		for( i = 0; i < 7; i++ )	// Find a free page
		{
			if( !(TABLE(TEMPORARY_DIR, TEMPORARY_PAGE+i) & 1) )
			{
				MM_RefPage( paddr );
				TABLE(TEMPORARY_DIR, TEMPORARY_PAGE+i) = (paddr & ~0xFFF) | 1;
				__asm__ __volatile__ ("invlpg (%0)" : : "r" (TEMPORARY_ADDR + (i<<12)) );
				//LogF("MM_MapTemp: RETURN 0x%x\n", TEMPORARY_ADDR + (i<<12));
				return TEMPORARY_ADDR + (i<<12);
			}
		}
		Proc_Yield();
	}
	return 0;
}

/**
 \fn void MM_FreeTemp(Uint addr)
 \brief Frees a tempoary map
 \param addr	Virtual Address
*/
void MM_FreeTemp(Uint addr)
{
	if( addr < TEMPORARY_ADDR )	return;
	if( addr > TEMPORARY_ADDR+(7<<12) )	return;
	//LogF("MM_FreeTemp: (addr=0x%x)\n", addr);
	MM_DerefPage( TABLE(TEMPORARY_DIR, ((addr>>12)&0x3FF)) );	// Derefernce
	TABLE(TEMPORARY_DIR, ((addr>>12)&0x3FF)) = 0;	// Unset
}

/**
 \fn void MM_SetCOW(Uint addr)
*/
void MM_SetCOW(Uint addr)
{
	if(!DIRA( addr ))	return;
	//Set Attributes
	if(TABLEA( addr ) & FLAG_WRITEABLE) {
		TABLEA( addr ) |= FLAG_CPYONWRITE;
		TABLEA( addr ) &= ~FLAG_WRITEABLE;
	}
}
