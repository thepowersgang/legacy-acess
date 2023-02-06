/*
AcessOS 0.1
Kernel Main
*/
#include <system.h>
#include <build.h>
#include <proc.h>
#include <proc_int.h>
#include <mboot.h>
#include <config.h>

#define USE_FDD	0

// ==== EXTERNAL FUNCTIONS ====
extern void fat_install();
extern void FDD1_Load();
extern int	Ne2k_Install();
extern t_proc_tss	*proc_tss;

// ==== GLOBALS ====
Uint32	giAcessVersion = 0x101;	//AcessOS v1

/**
 \fn void kmain(Uint32 mbMagic, mboot_info *mboot)
 \brief C section of kernel entrypoint
 \param mbMagic	Contents of eax at the start of the kernel
 \param mboot	Pointer to multiboot information structure
 
 This is the core C function in Acess and is the "parent"
 of the entire operating system.
*/
void kmain(Uint32 mbMagic, mboot_info *mboot)
{
	 int	pid;
	char	*argv[] = {"sh.axe", NULL};
	char	*envp[] = {"ACESS_VERSION=AcessOS 1.0", "PATH=/Mount/hda1", NULL};

	// Initialise Machine State
	gdt_install();
	IDT_Install();
	isrs_install();
	irq_install();
	
	// Start Text Logging
	VGAText_Install();
	
	// Print Acess Header
	LogF("====================================================================\n");
	LogF("--------------------------  Acess OS v0.4  -------------------------\n");
	LogF("====================================================================\n");
	
	// Install Timer
	time_install();
	// Install Keyboard
	Keyboard_Install();
	// Install Memory Manager
	memmanager_install((mboot->mem_upper + (0x100000>>10)) << 10);		//Supply Memory in Bytes
	// Initialise Heap
	Heap_Init();
	
	if(mbMagic == MULTIBOOT_MAGIC)
	{
		// Load ELF Symbols
		#if 0
		Debug_LoadELFSymtab(
			mboot->elf_shdr_num,
			mboot->elf_shdr_size,
			mboot->elf_shdr_addr + 0xC0000000,
			mboot->elf_shdr_shndx);
		#endif
		// Load Configuration from MBoot Structure
		Config_LoadMBoot(mboot);
	}
	else
	{
		// Loads Configuration from Defaults
		Config_LoadDefaults();
	}
	// Print out Configuration
	Config_PrintConfig();
	
	//LogF("=== Bringing up Multitasking.\n");
		Proc_Install();
		Syscall_Install();
		VM8086_Install();
	//LogF("--- Multitasking Started.\n");
	
	// Installs VFS and FileSystems
	//LogF("=== Installing Virtual Filesystem...\n");
		vfs_install();
			fat_install();
	//LogF("--- Virtual Filesystem Loaded.\n");
	
	// Install Drivers
	//LogF("=== Loading Drivers...\n");
		setup_disks();	// IDE PIO Driver - Not thread safe
		#if	USE_FDD
		FDD1_Load();	// Load FDD Driver
		#endif
		tty_install();	// Terminal
		PCI_Install();	// PCI Bus
		Vesa_Install();	// VBE/Vesa Driver
		PS2Mouse_Install();	// P/S2 Mouse Driver
		Ne2k_Install();	// RTL8139 Driver
	//LogF("--- Drivers Loaded.\n");
	
	// Mount Floppy
	#if USE_FDD
	vfs_mknod("/Mount/fd0", 1);
	vfs_mount("/Devices/fdd/0", "/Mount/fd0", 0xC);
	#endif
	
	//LogF("=== Loading Modules...\n");
	//	Module_Load("/Mount/hda1/kmod_bochsvbe.akm");
	//LogF("--- Modules Loaded.\n");
	
	LogF("=== Loading User Mode Shell.\n");
	pid = Proc_KFork();	// Use KFork to reduce overhead
	if(pid == 0)
	{
		Proc_Execve(gsInitProgram, argv, envp, 3);
		panic("Unable to start shell/init\n");
		return;
	}
	
	if(pid < 0)
		panic("Unable to Fork Kernel!\n");
	
	//LogF("--- User Shell Running. (PID #%i)\n", pid);
	
	//Proc_WaitPid(pid, 0);
	
	//LogF("--- Shell Completed, rebooting\n");
	for (;;)	Proc_Sleep();	// Yeild Until Message
}
