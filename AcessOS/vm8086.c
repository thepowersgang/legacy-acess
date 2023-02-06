/*
AcessOS 0.1
VM8086 Task Creation Code
*/
#include <system.h>
#include <proc_int.h>

// === CONSTANTS ===
#define STACK_SEG	0x9F00
#define STACK_OFS	0x0AFE

// === IMPORTS ===
extern t_proc	*gpCurrentProc;
extern void Proc_ProcessStart(Uint *stack);

// === GLOBALS ===
int	gbVM8086_Pid = -1;
int gbVM8086_InUse = 0;
int	gbVM8086_MemPtr = 0;
volatile int	gbVM8086_Complete = 0;
volatile int	giVM8086_Callee = -1;
volatile tRegs16	gVM8086_Ret;

// === CONSTANTS ===
// pushf; pop es; pop ds; far ret
static const char REAL_MODE_STUB[] = {0x9C, 0x07, 0x1F, 0xCB};

// === CODE ===
/**
 \fn void VM8086_Lock()
 \brief Acquires the spinlock for the VM8086 Monitor
*/
void VM8086_Lock()
{	
	// Acquire Spinlock
	while(gbVM8086_InUse)	Proc_Yield();
	gbVM8086_InUse = 1;
	gbVM8086_MemPtr = 0x500;
}

/**
 \fn void VM8086_Unlock()
 \brief Frees the spinlock
*/
void VM8086_Unlock()
{
	gbVM8086_InUse = 0;
	gbVM8086_MemPtr = 0;
}

/**
 \fn void *VM8086_Allocate(int bytes)
 \brief Fetch a block of memory in real mode area
*/
void *VM8086_Allocate(int bytes)
{
	if(gbVM8086_MemPtr == 0)
		return NULL;
	gbVM8086_MemPtr += bytes;
	return (void*)(gbVM8086_MemPtr - bytes + 0xC0000000);
}

/**
 \fn void *VM8086_Linear(Uint16 seg, Uint16 ofs)
 \brief Returns a linear address given a segment and offset
*/
void *VM8086_Linear(Uint16 seg, Uint16 ofs)
{
	return (void*)(0xC0000000 + (seg << 4) + ofs);
}

/**
 \fn void VM8086_GPF(t_regs *r)
 \brief Handles a General Protection fault
*/
void VM8086_GPF(t_regs *r)
{
	Uint8	opcode;
	
	opcode = *(Uint8*)((r->cs<<4)+(r->eip&0xFFFF));
	//LogF("[0x%x:%x] 0x%x\n", r->cs, r->eip&0xFFFF, opcode);
	
	if(r->eip == 0x10 && r->cs == 0xFFFF && opcode == 0x9C)
	{
		//LogF("VM8086 - Interrupt completed, stopping\n");
		gVM8086_Ret.ax = r->eax;	gVM8086_Ret.bx = r->ebx;
		gVM8086_Ret.cx = r->ecx;	gVM8086_Ret.dx = r->edx;
		gVM8086_Ret.si = r->esi;	gVM8086_Ret.di = r->edi;
		gVM8086_Ret.bp = r->ebp;
		gVM8086_Ret.ds = r->ds;		gVM8086_Ret.es = r->es;
		gbVM8086_Complete = 1;
		
		Proc_Sleep();	// Yield to monitor
		while(gbVM8086_Complete)	Proc_Yield();	// Yield to monitor
		Proc_WakeProc(giVM8086_Callee);
		//LogF("Child Restarting\n");
		
		// Rebuild Stack
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = 0xFFFF;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = 0x10;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = gVM8086_Ret.cs;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = gVM8086_Ret.ip;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = gVM8086_Ret.ds;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = gVM8086_Ret.es;
		
		// Set Registers
		r->eip = 0x11;	r->cs = 0xFFFF;
		r->eax = gVM8086_Ret.ax;	r->ebx = gVM8086_Ret.bx;
		r->ecx = gVM8086_Ret.cx;	r->edx = gVM8086_Ret.dx;
		r->esi = gVM8086_Ret.si;	r->edi = gVM8086_Ret.di;
		r->ebp = gVM8086_Ret.bp;
		r->ds = 0x23;	r->es = 0x23;
		r->fs = 0x23;	r->gs = 0x23;
		return;
	}
	
	switch(opcode)
	{
	case 0x9C:	//PUSHF
		r->esp -= 2;
		r->eip ++;
		*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = r->eflags & 0xFFFF;
	//	LogF("PUSHF Emulated\n");
		break;
	case 0x9D:	//POPF
		r->eip ++;
		r->eflags &= 0xFFFF0002;
		r->eflags |= *(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) & 0xFFFD;	// Changing IF is not allowed
		r->esp += 2;
	//	LogF("POPF Emulated\n");
		break;
	
	case 0xCD:	//INT imm8
		{
		int id;
		r->eip ++;
		id = *(Uint8*)(0xC0000000+(r->cs<<4)+(r->eip&0xFFFF));
		r->eip ++;
		
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = r->cs;
		r->esp -= 2;	*(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) ) = r->eip;
		
		r->cs = *(Uint16*)(0xC0000000+4*id+2);
		r->eip = *(Uint16*)(0xC0000000+4*id);
		//LogF("INT 0x%x\n", id);
		}
		break;
	
	case 0xCF:	//IRET
		r->eip = *(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) );	r->esp += 2;
		r->cs  = *(Uint16*)( (r->ss<<4) + (r->esp&0xFFFF) );	r->esp += 2;
		//LogF("IRET to 0x%x:%x\n", r->cs, r->eip);
		break;
		
	case 0xEC:	//OUT AL, DX
		r->eip++;
		r->eax &= 0xFFFFFF00;
		r->eax |= inportb(r->edx&0xFFFF);
		//LogF("IN AL, DX Emulated (in al, 0x%x)\n", r->edx&0xFFFF);
		break;
	case 0xED:	//IN AX, DX
		r->eip ++;
		r->eax &= 0xFFFF0000;
		r->eax |= inportw(r->edx&0xFFFF);
		//LogF("IN AX, DX Emulated (in ax, 0x%x)\n", r->edx&0xFFFF);
		break;
		
	case 0xEE:	//OUT DX, AL
		r->eip++;
		outportb(r->edx&0xFFFF, r->eax&0xFF);
		//LogF("OUT DX, AL Emulated (out 0x%x, 0x%x)\n", r->edx&0xFFFF, r->eax&0xFFFF);
		break;
	case 0xEF:	//OUT DX, AX
		r->eip ++;
		outportw(r->edx&0xFFFF, r->eax&0xFFFF);
		//LogF("OUT DX, AX Emulated (out 0x%x, 0x%x)\n", r->edx&0xFFFF, r->eax&0xFF);
		break;
		
	case 0xFA:	//CLI
		r->eip++;
		break;
	case 0xFB:	//STI
		r->eip++;
		break;
		
	default:
		panic("VM8086_GPF - Unknown opcode (0x%x)\n", opcode);
		break;
	}
}

/**
 \fn int VM8086_Int(int id, tRegs16 *r)
 \brief Preforms a real mode interrupt
*/
int VM8086_Int(int id, tRegs16 *r)
{	
	// Validate Input
	if(id < 0 || id > 255)	return -1;
	
	// Check Spinlock - To hopefully thwart non-safe code
	if(!gbVM8086_InUse) {
		warning("VM8086_Int - Spinlock not set, returning.");
		return -2;
	}
	
	// Set CS:IP
	gVM8086_Ret.cs = *(Uint16*)(0xC0000000+4*id+2);
	gVM8086_Ret.ip = *(Uint16*)(0xC0000000+4*id);
	
	// Set Regs
	gVM8086_Ret.ax = r->ax;	gVM8086_Ret.bx = r->bx;
	gVM8086_Ret.cx = r->cx;	gVM8086_Ret.dx = r->dx;
	gVM8086_Ret.si = r->si;	gVM8086_Ret.bp = r->bp;
	gVM8086_Ret.di = r->di;
	gVM8086_Ret.ds = r->ds;	gVM8086_Ret.es = r->es;
	
	// Save Callee
	giVM8086_Callee = proc_pid;
	// Unset completion flag
	gbVM8086_Complete = 0;
	// Wake Process
	Proc_WakeProc(gbVM8086_Pid);
	
	// Wait for task to finish
	//while(!gbVM8086_Complete)
	//	Proc_Yield();
	Proc_Sleep();
	
	
	//LogF("Child Terminated\n");
	
	// Save Registers
	r->ax = gVM8086_Ret.ax;	r->bx = gVM8086_Ret.bx;
	r->cx = gVM8086_Ret.cx;	r->dx = gVM8086_Ret.dx;
	r->si = gVM8086_Ret.si;	r->bp = gVM8086_Ret.bp;
	r->di = gVM8086_Ret.di;
	r->ds = gVM8086_Ret.ds;	r->es = gVM8086_Ret.es;
	return 1;
}

/**
 * \fn void VM8086_Install()
 */
void VM8086_Install()
{
	int pid;
	gbVM8086_Complete = 1;
	
	pid = Proc_KFork();
	if(pid == 0)	// Child
	{
		Uint	*stacksetup;
		Uint16	*rmstack;
		 int	i;
		 
		// Set Image Name
		gpCurrentProc->ImageName = "VM8086";
		// Map Memory
		for(i=0;i<0x60;i++) {
			mm_map( 0xA0000+i*0x1000, 0xA0000+i*0x1000 );
			mm_setro( 0xA0000+i*0x1000, 1 );	// Set Read Only
		}
		for(i=0;i<0xA0;i++)
			mm_map( i * 0x1000, i * 0x1000 );
		mm_alloc( 0x100000 );	// System Stack / Stub
		memcpy( (void*)0x100000, REAL_MODE_STUB, sizeof(REAL_MODE_STUB)/sizeof(REAL_MODE_STUB[0]) );
		
		//LogF(" VM8086_Int: Building Stack\n");
		rmstack = (Uint16*)((STACK_SEG<<4)+STACK_OFS);
		*rmstack-- = 0xFFFF;	//CS
		*rmstack-- = 0x10;	//IP
		
		// Setup Stack
		stacksetup = (Uint*)0x101000;
		stacksetup --;	stacksetup --;
		stacksetup --;	stacksetup --;
		*stacksetup-- = STACK_SEG;	//DS
		*stacksetup-- = STACK_SEG;	//ES
		*stacksetup-- = STACK_SEG;	//SS
		*stacksetup-- = STACK_OFS-2;	//SP
		*stacksetup-- = 0x20202;	//FLAGS
		*stacksetup-- = 0xFFFF;	//CS
		*stacksetup-- = 0x10;	//IP
		*stacksetup-- = 0xA;	//AX
		*stacksetup-- = 0xC;	//CX
		*stacksetup-- = 0xD;	//DX
		*stacksetup-- = 0xB;	//BX
		*stacksetup-- = 0x54;	//SP
		*stacksetup-- = 0xB4;	//BP
		*stacksetup-- = 0x51;	//SI
		*stacksetup-- = 0xD1;	//DI
		*stacksetup-- = 0x20|3;	//DS - Kernel
		*stacksetup-- = 0x20|3;	//ES - Kernel
		*stacksetup-- = 0x20|3;	//FS
		*stacksetup-- = 0x20|3;	//GS
		stacksetup ++;

		Proc_ProcessStart(stacksetup);
		return;
	}
	if(pid > 0)		// Parent
	{
		gbVM8086_Pid = pid;
		Proc_Yield();
		LogF("[PROC] VM8086 BIOS Handler Started\n");
		return;
	}
	panic("VM8086_Install - Failed to fork kernel\n");
	return;
}
