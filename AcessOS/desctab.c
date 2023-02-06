/*
 AcessOS 0.1
 Desctiptor Table Manager
 
 Manages the IDT and GDT as well as
 handling IRQs.
*/
#include "system.h"

//DEFINES
/**
 \def ISRS_ENABLED
 \brief Debug flag that enables/disables ISRs
*/
#define	ISRS_ENABLED	1
/**
 \def GDT_SIZE
 \brief Size of the Global Descriptor Table
*/
#define GDT_SIZE	(5+MAX_CPUS)
#define IDT_SIZE	255

//STRUCTURES
/**
 \struct idt_entry
 \brief IDT Entry Structure
*/
struct idt_entry
{
    Uint16	base_lo;	//!< Low 16 Bits of address
    Uint16	sel;	//!< Selector (CS)
    Uint8	always0;	//!< Reserved. Always 0
    Uint8	flags;	//!< Flags
    Uint16	base_hi;	//!< High 16 Bits of address
} __attribute__((packed));

/**
 \struct gdt_entry
 \brief GDT Entry Structure
*/
struct gdt_entry {
    Uint16 limit_low;	//!< Low 16 Bits of limit
    Uint16 base_low;	//!< Low 16 Bits of base
    Uint8 base_middle;	//!< Bits 16-23 of base
    Uint8 access;	//!< Access Flags
    Uint8 granularity;	//!< Granuality & High 4 bits of limit
    Uint8 base_high;	//!< High 8 Bits of base
} __attribute__((packed));

/**
 \struct table_ptr
 \brief Pointer Structure
*/
struct table_ptr {
	Uint16 limit;	//!< Table Limit (Size)
	Uint32 base;	//!< Table Base (Address)
} __attribute__((packed));

//IMPORTS
extern void gdt_flush();		//Import from start.asm
extern void tss_setup();	// Fill the TSS
extern void idt_load();	//In start.asm
extern void MM_PageFault(Uint addr, t_regs *r);
extern void Proc_DumpRunning();
//Interrupt Handlers for errors (first 32 IDT entries)
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
//Linker Variables
extern int	gLdStrtab, gLdStabs, gLdStabsEnd;

//GLOBALS
struct gdt_entry	gdt[GDT_SIZE];		//4 GDT Entries
struct table_ptr	gp;			//GDT Pointer
struct idt_entry	idt[IDT_SIZE];
struct table_ptr	idtp;
//Error Messages
static char *exception_messages[] = {
	"Division By Zero",	"Debug", "Non Maskable Interrupt", "Breakpoint",
	"Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",

	"Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
	"Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",

	"Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",

	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved"
};

// === CODE ===
/**
 \fn void gdt_set_gate(int num, Uint32 base, Uint32 limit, Uint8 access, Uint8 gran);
 \brief Setup a descriptor in the Global Descriptor Table
 \param num	integer - 0 Based idex of gate
 \param base	Address - Linear Base of Segment
 \param limit	Integer - Size of Segment
 \param access	Integer - Access Masks
 \param gran		Integer - Granularity
*/
void gdt_set_gate(int num, Uint32 base, Uint32 limit, Uint8 access, Uint8 gran)
{
    //Base Address
    gdt[num].base_low = (base & 0xFFFF);		//Base Low (Low 16 bits)
    gdt[num].base_middle = (base >> 16) & 0xFF;	//Base Middle (Mask 0xFF0000)
    gdt[num].base_high = (base >> 24) & 0xFF;	//Base High (High 8 Bits)

    //Limit (Part of it is in gran)
    gdt[num].limit_low = (limit & 0xFFFF);		//Limit Low 16 Bits
    gdt[num].granularity = ((limit >> 16) & 0x0F);	//Limit High 4 Bits

	//Set Gran. and Access
    gdt[num].granularity |= (gran & 0xF0);		//Granularity (High 4 bits only)
    gdt[num].access = access;			//Set Access
}

/**
 \fn void gdt_install()
 \brief Called by main - Installs the GDT
 
 Should be called by main. This will setup the special GDT
 pointer, set up the first 3 entries in our GDT, and then
 finally call gdt_flush() in our assembler file in order
 to tell the processor where the new GDT is and update the
 new segment registers.
*/
void gdt_install()
{
    gp.limit = (sizeof(struct gdt_entry) * GDT_SIZE) - 1;	//Set Limit
    //gp.base = (unsigned int) &gdt - 0xC0000000;		//Set Pointer
    gp.base = (unsigned int) &gdt;		//Set Pointer

    gdt_set_gate(0, 0, 0, 0, 0);		//Our NULL descriptor
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);		//Code Segment (Base:0,Limit:4GiB,CODE)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);		//Data Segment (Base:0,Limit:4GiB,DATA)
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);		//Usermode Code Segment (Base:0,Limit:4GiB,CODE)
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);		//Usermode Data Segment (Base:1MiB,Limit:4GiB,DATA)
	
	//Set Tss
	tss_setup();
	//Flush GDT
    gdt_flush();
}

/**
 \fn void IDT_SetGate(Uint8 num, Uint32 base, Uint16 sel, Uint8 flags)
 \brief Set an IDT entry
 \param num	integer - Entry Number
 \param base	integer - Entry Base Address
 \param sel	integer - Selector
 \param flags	integer - Flags
  - (bit 7:	Present)
  - (6,5:	Privilage - CPL)
  - (4:		System Segment)
  - (3-0:	Type:
	5h	Task Gate
	6h	16-bit interrupt gate
	7h	16-bit trap gate
	Eh	32-bit interrupt gate
	Fh	32-bit trap gate
	)
*/
void IDT_SetGate(Uint8 num, Uint32 base, Uint16 sel, Uint8 flags)
{
    //Set Base Address
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;

    idt[num].sel = sel;	//Selector
    idt[num].always0 = 0;	//Unused (Always NULL)
    idt[num].flags = flags;	//Flags
}

/**
 \fn void IDT_Install()
 \brief Installs IDT
*/
void IDT_Install()
{
	//Setup IDT Pointer
    idtp.limit = (sizeof (struct idt_entry) * IDT_SIZE) - 1;
    idtp.base = (unsigned int) &idt;

	//Initalize IDT with zeroes
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_SIZE);
    //Load IDT
    idt_load();
}

/**
 \fn void isrs_install()
 \brief Installs the ISRs
 
 Binds all error routines to their respective
 IDT entries.
*/
void isrs_install()
{
	IDT_SetGate(13, (unsigned)isr13, 0x08, 0x8E);	// GPF - Always needed
	#if ISRS_ENABLED
    IDT_SetGate( 0, (unsigned)isr0, 0x08, 0x8E);	IDT_SetGate( 1, (unsigned)isr1, 0x08, 0x8E);
    IDT_SetGate( 2, (unsigned)isr2, 0x08, 0x8E);	IDT_SetGate( 3, (unsigned)isr3, 0x08, 0x8E);
    IDT_SetGate( 4, (unsigned)isr4, 0x08, 0x8E);	IDT_SetGate( 5, (unsigned)isr5, 0x08, 0x8E);
    IDT_SetGate( 6, (unsigned)isr6, 0x08, 0x8E);	IDT_SetGate( 7, (unsigned)isr7, 0x08, 0x8E);

    IDT_SetGate( 8, (unsigned)isr8, 0x08, 0x8E);	IDT_SetGate( 9, (unsigned)isr9, 0x08, 0x8E);
    IDT_SetGate(10, (unsigned)isr10, 0x08, 0x8E);	IDT_SetGate(11, (unsigned)isr11, 0x08, 0x8E);
    IDT_SetGate(12, (unsigned)isr12, 0x08, 0x8E);
    IDT_SetGate(14, (unsigned)isr14, 0x08, 0x8E);	IDT_SetGate(15, (unsigned)isr15, 0x08, 0x8E);

    IDT_SetGate(16, (unsigned)isr16, 0x08, 0x8E);	IDT_SetGate(17, (unsigned)isr17, 0x08, 0x8E);
    IDT_SetGate(18, (unsigned)isr18, 0x08, 0x8E);	IDT_SetGate(19, (unsigned)isr19, 0x08, 0x8E);
    IDT_SetGate(20, (unsigned)isr20, 0x08, 0x8E);	IDT_SetGate(21, (unsigned)isr21, 0x08, 0x8E);
    IDT_SetGate(22, (unsigned)isr22, 0x08, 0x8E);	IDT_SetGate(23, (unsigned)isr23, 0x08, 0x8E);

    IDT_SetGate(24, (unsigned)isr24, 0x08, 0x8E);	IDT_SetGate(25, (unsigned)isr25, 0x08, 0x8E);
    IDT_SetGate(26, (unsigned)isr26, 0x08, 0x8E);	IDT_SetGate(27, (unsigned)isr27, 0x08, 0x8E);
    IDT_SetGate(28, (unsigned)isr28, 0x08, 0x8E);	IDT_SetGate(29, (unsigned)isr29, 0x08, 0x8E);
    IDT_SetGate(30, (unsigned)isr30, 0x08, 0x8E);	IDT_SetGate(31, (unsigned)isr31, 0x08, 0x8E);
	#endif
}

/**
 \fn void Fault_Handler(struct regs *r)
 \brief Called when ever an error is encounted
 \param r	registers - Register state at error
 
 Prints the registers and an error message and then halts the system
*/
void Fault_Handler(t_regs *r)
{
	#if ISRS_ENABLED
	Uint	addr;

	if(r->int_no == 1)
	{
//#if USE_DEBUG_EXCEPTION
		VGAText_SetAttrib(CLR_YELLOW|CLR_FLAG);	// Red on Black
		
		LogF("Debug Exception - Non-Fatal\n");
		Debug_Backtrace(r->eip, r->ebp);
		Debug_Disasm(r->eip);
		warning(
			"EAX: %08x\tEBX: %08x\tECX:%08x\tEDX:%08x\n"
			"EIP: %08x\tCS: %04x\tEFLAGS: %08x\tESP: %08x\n",
			r->eax, r->ebx, r->ecx, r->edx,
			r->eip, r->cs, r->eflags, r->esp);
//#else
		r->eflags &= ~(1<<8);
		
//#endif
		return;
	}
	#endif
	
	if(r->int_no == 13 && r->eflags & 0x20000)
	{
		VM8086_GPF(r);
		return;
	}
	
	#if ISRS_ENABLED
	if (r->int_no < 32)
	{
		switch(r->int_no)
		{
		case 14:
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (addr) :);
			//warning("Page Faut: PID%i CR2 = 0x%x\n", proc_pid, addr);
			MM_PageFault(addr, r);
			return;
		case 10:	// Bad TSS
		case 11:	// Segment Not Present
		case 12:	// Stack Fault
		case 13:	// GP Fault
			VGAText_SetAttrib(CLR_RED|CLR_FLAG);	// Red on Black
			
			LogF("%s Exception, System Halted\n", exception_messages[r->int_no]);
			if(r->eflags & 0x20000)
			{
				Debug_Disasm((r->cs<<4)+r->eip);
			}
			else
			{
				Debug_Backtrace(r->eip, r->ebp);
				Debug_Disasm(r->eip);
			}
			
			Proc_DumpRunning();
			
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (addr) :);
			LogF(
				"CS:EIP = 0x%02x:%08x\tSS:ESP = 0x%02x:%08x\n"
				"EAX: 0x%08x\tEBX: 0x%08x\tECX: 0x%08x\tEDX: 0x%08x\n"
				"EDI: 0x%08x\tESI: 0x%08x\tEBP: 0x%08x\n"
				"GS: 0x%04x\tFS: 0x%04x\tES: 0x%04x\tDS: 0x%04x\n"
				"EFLAGS: 0x%08x\tCR2: 0x%08x\n"
				"\n"
				"Error Code:\n"
				" Selector: 0x%x\n"
				" From LDT?: %B, From IDT?: %B, External Event?: %B\n",
				r->cs, r->eip, r->ss, r->esp,
				r->eax, r->ebx, r->ecx, r->edx,
				r->edi, r->esi, r->ebp,
				r->gs, r->fs, r->es, r->ds,
				r->eflags, addr,
				(r->err_code&0xFFFF)>>3,
				r->err_code&4, r->err_code&2, r->err_code&1
				);
			panic("Kernel Crashed\n");
			break;
		default:
			panic(
				"%s Exception, System Halted\n"
				"CS:EIP = 0x%02x:%08x\tSS:ESP = 0x%02x:%08x\n"
				"EAX: 0x%08x\tEBX: 0x%08x\tECX: 0x%08x\tEDX: 0x%08x\n"
				"EDI: 0x%08x\tESI: 0x%08x\tEBP: 0x%08x\n"
				"GS: 0x%04x\tFS: 0x%04x\tES: 0x%04x\tDS: 0x%04x\n"
				"EFLAGS: 0x%08x\n",
				exception_messages[r->int_no],
				r->cs, r->eip, r->ss, r->esp,
				r->eax, r->ebx, r->ecx, r->edx,
				r->edi, r->esi, r->ebp,
				r->gs, r->fs, r->es, r->ds,
				r->eflags
				);
			break;
		}
		__asm__ ("cli" ::);
		for (;;)
			__asm__ ("hlt" ::);
	}
	#endif
}

