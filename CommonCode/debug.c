/*
AcessOS/AcessBasic 0.1
Kernel Debug Code
*/
#include <acess.h>
#include <reloc_elf.h>

#define MAX_BACKTRACE	5

#define DEBUG	1

// === FUNCTION DEFS ===
extern void sched();
extern void FDD_WaitIRQ();
extern void Vesa_Int_SetMode();
extern void Vesa_Ioctl();
extern void devfs_ioctl();
extern void syscall_kexec();
extern void Fault_Handler();
extern void VM8086_GPF();
extern void	gKernelCodeEnd;
// ==== GLOBAL DATA ====
typedef struct {
	char	*name;
	Uint	addr;
} tDbgSym;
tDbgSym gBuiltinDebugSyms[] = {
	// Proc Man
	{"_sched", (Uint)sched},
	{"_Proc_Yield", (Uint)Proc_Yield},
	{"_Proc_Execve", (Uint)Proc_Execve},
	{"_Proc_Open", (Uint)Proc_Open},
	{"_Proc_Sleep", (Uint)Proc_Sleep},
	// Error Handler
	{"_Fault_Handler", (Uint)Fault_Handler},
	// VM8086
	{"_VM8086_Int", (Uint)VM8086_Int},
	{"_VM8086_GPF", (Uint)VM8086_GPF},
	// FDD
	{"_FDD_WaitIRQ", (Uint)FDD_WaitIRQ},
	// Vesa
	{"_Vesa_Int_SetMode", (Uint)Vesa_Int_SetMode},
	{"_Vesa_Ioctl", (Uint)Vesa_Ioctl},
	// DevFS
	{"_devfs_ioctl", (Uint)devfs_ioctl},
	// VFS
	{"_vfs_ioctl", (Uint)vfs_ioctl},
	// Syscalls
	{"_syscall_kexec", (Uint)syscall_kexec},
	// Kernel End
	{"DATA", (Uint)&gKernelCodeEnd}
	};
tDbgSym *gDebugSyms = gBuiltinDebugSyms;
 int	giDebugSymCount = sizeof(gBuiltinDebugSyms)/sizeof(gBuiltinDebugSyms[0]);

// ==== CODE ====
void Debug_LoadELFSymtab(Uint32 num, Uint32 spe, Uint32 addr, Uint32 shndx)
{
	 int	i, count = 0;
	elf_shent	*shTable;
	char		*shStrTable;
	elf_symtab	*symTab = NULL;
	char		*strTab = NULL;
	
	#if DEBUG
		LogF("Debug_LoadELFSymtab: (num=%i,spe=%i,addr=0x%x,shndx=%i)\n", num, spe, addr, shndx);
	#else
		LogF("Loading ELF Symbol Table...");
	#endif
	
	shTable = (elf_shent*)addr;
	shStrTable = (char*)shTable[shndx].address;
	shStrTable += 0xC0000000;
	
	for(i = 0; i < num; i++)
	{
		#if DEBUG
		LogF(" %i: %i'%s' Ofs:0x%x, Addr:0x%x,Size:0x%x\n",
			i,
			shTable[i].name,
			shStrTable + shTable[i].name,
			shTable[i].offset,
			shTable[i].address,
			shTable[i].size);
		#endif
			
		if(strcmp(shStrTable + shTable[i].name, ".symtab") == 0)
		{
			symTab = (elf_symtab*)(shTable[i].address + 0xC0100000);
			count = shTable[i].size / sizeof(elf_symtab);
			continue;
		}
		
		if(strcmp(shStrTable + shTable[i].name, ".strtab") == 0)
		{
			strTab = (char*)(shTable[i].address + 0xC0000000);
			continue;
		}
	}
	if( !symTab || !strTab || !count)
		return;
	#if DEBUG
		LogF(" count = %i\n", count);
		LogF(" strTab = (char*)0x%08x\n", strTab);
	#endif
	
	// Create Symbol Table
	giDebugSymCount = 0;
	gDebugSyms = malloc(count*sizeof(*gDebugSyms));
	
	for(i = 0; i < count; i++)
	{
		if(symTab[i].value == 0)
			continue;
		#if DEBUG
		LogF(" %3i: 0x%x = %i'%s'\n", i, symTab[i].value, symTab[i].name, strTab + symTab[i].name);
		#endif
		gDebugSyms[giDebugSymCount].name = malloc(strlen(strTab + symTab[i].name) + 1);
		strcpy(gDebugSyms[giDebugSymCount].name, strTab + symTab[i].name);
		gDebugSyms[giDebugSymCount].addr = symTab[i].value;
		giDebugSymCount ++;
	}
	
	gDebugSyms = realloc(giDebugSymCount * sizeof(*gDebugSyms), gDebugSyms);
	
	#if DEBUG
	#else
		LogF("Done\n");
	#endif
	
}

char *Debug_GetSymbol(Uint addr, Uint *delta)
{
	char	*ret = NULL;
	 int	i;
	*delta = -1;
	for(i = 0; i < giDebugSymCount; i++)
	{
		//LogF("0x%x==0x%x - '%s'\n", addr, gDebugSyms[i].addr, gDebugSyms[i].name);
		if(gDebugSyms[i].addr == addr)
		{
			*delta = 0;
			return gDebugSyms[i].name;
		}
		if(gDebugSyms[i].addr < addr && addr - gDebugSyms[i].addr < *delta)
		{
			*delta = addr - gDebugSyms[i].addr;
			//LogF(" 0x%x > 0x%x - '%s', *delta=0x%x\n", addr, gDebugSyms[i].addr, gDebugSyms[i].name, *delta);
			ret = gDebugSyms[i].name;
		}
	}
	return ret;
}

void Debug_Backtrace(Uint eip, Uint ebp)
{
	int		i = 0;
	Uint	delta;
	char	*str;
	
	//if(eip < 0xC0000000 && eip > 0x1000)
	//{
	//	LogF("Backtrace: User - 0x%x\n", eip);
	//	return;
	//}
	
	if(eip > 0xE0000000)
	{
		LogF("Backtrace: Data Area - 0x%x\n", eip);
		return;
	}
	if(eip > 0xC8000000)
	{
		LogF("Backtrace: Kernel Module - 0x%x\n", eip);
		return;
	}
	
	str = Debug_GetSymbol(eip, &delta);
	if(str == NULL)
		LogF("Backtrace: 0x%x", eip);
	else
		LogF("Backtrace: %s+0x%x", str, delta);
	if(!MM_IsValid(ebp))
	{
		LogF("\nBacktrace: Invalid EBP, stopping\n");
		return;
	}
	
	while(0xE8000000 < ebp && ebp < 0xF0000000 && i < MAX_BACKTRACE)
	{
		str = Debug_GetSymbol(*(Uint*)(ebp+4), &delta);
		if(str == NULL)
			LogF(" >> 0x%x", *(Uint*)(ebp+4));
		else
			LogF(" >> %s+0x%x", str, delta);
		ebp = *(Uint*)ebp;
		i++;
	}
	LogF("\n");
}

void Debug_Disasm(Uint eip)
{
	Uint8	byte;
	
	if(!MM_IsValid(eip))
		return;
	
	LogF("[0x%x]: ", eip);
fetchOpcode:
	byte = *(Uint8*)eip;
	switch(byte)
	{
	case 0x03:
		LogF("add ");
		byte = *(Uint8*)(eip+1);
		switch(byte)
		{
		case 0x46:	LogF("eax, [esi+0x%x]", *(Uint8*)(eip+2));	break;
		default:	LogF("UNK (0x3,0x%x)", byte);	break;
		}
		break;
	case 0x07:
		LogF("pop es");
		break;
	
	case 0x0F:
		byte = *(Uint8*)(eip+1);
		switch(byte)
		{
		case 0xa9:	LogF("pop gs");	break;
		default:	LogF("UNK (0xF,0x%x)", byte);	break;
		}
		break;
		
	case 0x1F:	LogF("pop ds");	break;
		
	case 0x89:
		byte = *(Uint8*)(eip+1);
		switch(byte)
		{
		case 0x43:	LogF("mov [ebx+0x%x], eax", *(Uint8*)(eip+2));	break;
		default:	LogF("UNK (0x89,0x%x)", byte);	break;
		}
		break;
		
	case 0x8B:
		byte = *(Uint8*)(eip+1);
		switch(byte)
		{
		case 0x00:	LogF("mov eax, [eax]");	break;
		case 0x40:	LogF("mov eax, [eax+0x%x]", *(Uint8*)(eip+2));	break;
		case 0x83:	LogF("mov eax, [ebx+0x%x]", *(Uint32*)(eip+2));	break;
		default:	LogF("UNK (0x8B,0x%x)", byte);	break;
		}
		break;
	
	case 0x9C:
		LogF("pushf");
		break;
		
	case 0xA1:
		LogF("mov eax, DS:[0x%x]", *(Uint32*)(eip+1));
		break;
	case 0xA4:	LogF("movsb");	break;
		
	case 0xCD:	LogF("int 0x%x", *(Uint8*)(eip+1));	break;
	case 0xCF:	LogF("iret");	break;
	
	case 0xF3:	LogF("rep ");	eip++;	goto fetchOpcode;
	
	case 0xFF:
		byte = *(Uint8*)(eip+1);
		switch(byte)
		{
		case 0x30:	LogF("push DWORD DS:[EAX]");	break;
		default:	LogF("UNK (0xFF,0x%x)", byte);	break;
		}
		break;
	default:
		LogF("Unknown Opcode 0x%x", byte);
		break;
	}
	LogF("\n");
	return;
}
