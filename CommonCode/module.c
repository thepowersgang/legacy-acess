/*
Acess (Common Code)
Module Loader
*/
#include <acess.h>
#include <proc.h>

#define MODULE_DEBUG	1
#define DEBUG	(ACESS_DEBUG | MODULE_DEBUG)

//=== TYPEDEFS ===
typedef void (* t_unload )(void);

//=== STRUCTURES ===
typedef struct {
	char	ident[4];
	t_unload	unload;
	Uint32	*base;
} t_module;

//=== GLOBALS ===
int	module_count = 0;
t_module	*modules;
//=== IMPORTS ===
extern Uint	k_export_magic;

//=== PROTOTYPES ===
t_executable *amod_load(int fp);
t_executable *elf_open(int fp);
t_executable *PE_Open(int fp);

//=== CODE ===
/*
int Module_Load(char *file)
- Loads a module from a file
*/
int Module_Load(char *file)
{
	Uint32	pbase = -1;
	Uint32	rbase = -1;
	Uint32	size = 0;
	 int	fp = -1;
	 int	ret = -1;
	 int	i;
	t_executable	*info;
	
	#if DEBUG
		LogF("Module_Load: (file=\"%s\")\n", file);
	#endif
	
	// - Open File -
	fp = vfs_open(file, VFS_OPENFLAG_READ);
	if(fp == -1)	return -1;
	
	#if DEBUG
		LogF(" Module_Load: fp=0x%x\n", fp);
	#endif
	
	info = LoadExecutable(fp);
	if(info == NULL)
	{
		warning("Module_Load - Unable to load module\n");
		vfs_close(fp);
		return -1;
	}
	
	#if DEBUG
		LogF(" Module_Load: File Scanned\n");
	#endif
	
	//Check for position independence
	if(!(info->flags & EXE_P_RELOC))
	{
		warning("Module_Load: Non relocatable executable\n");
		FreeExecutable(info);
		vfs_close(fp);
		return -1;
	}
	
	// - Determine Executable's Prefered Base and size -
	size = info->sects[0].rva + info->sects[0].size;
	for(i=0;i<info->sectCount;i++)
	{
		if(info->sects[i].rva + info->sects[i].size > size)
			size = info->sects[i].rva + info->sects[i].size;
	}
	pbase = info->PreferedBase;
	size -= pbase;
	
	// - Check if Module execeeds size limit -
	if(size > 0x4000)
	{
		warning("Module_Load: `%s' is too large to load (0x%x > 0x4000)\n", file, size);
		FreeExecutable(info);
		vfs_close(fp);
		return -1;
	}
	
	#if DEBUG
		LogF(" Module_Load: entrypoint=0x%x\n", info->entrypoint);
		LogF(" Module_Load: pbase=0x%x,size=0x%x\n", pbase, size);
		LogF(" Module_Load: sectCount=%i\n", info->sectCount);
	#endif
	
	// = Load Executable =
	rbase = (Uint32) MM_AllocRange(0xD0, 4);
	#if DEBUG
		LogF(" Module_Load: rbase=0x%x\n", (Uint)rbase);
	#endif
	
	// - Check if allocation failed - 
	if(rbase == 0) {
		warning("Module_Load: Unable to allocate memory for module\n");
		FreeExecutable(info);
		vfs_close(fp);
		return -1;
	}
	
	// - Load Sections -
	for(i=0;i<info->sectCount;i++)
	{
		info->sects[i].rva += rbase - pbase;
		#if DEBUG
			LogF(" Module_Load: Section #%i, `%s'\n", i, info->sects[i].name);
			LogF("  Module_Load: A:0x%x,S:0x%x,Dest:0x%x\n",
				info->sects[i].rva, info->sects[i].size, info->sects[i].rva);
		#endif
		if(info->sects[i].size == 0)
			continue;
		vfs_seek(fp, info->sects[i].offset, SEEK_SET);
		vfs_read(fp, info->sects[i].size, (void*)(info->sects[i].rva));
	}
	// - Close File -
	vfs_close(fp);
	
	// - Relocate Executable -
	info->ActualBase = rbase;	// Set Executable base for relocator
	RelocateExecutable(info);
	// - Free Executable Structure -
	FreeExecutable(info);
	// - Close File -
	vfs_close(fp);
	
	#if DEBUG
		LogF(" Module_Load: Executing from 0x%x\n", (Uint)(rbase-pbase+info->entrypoint) );
	#endif
	// = Execute Module =
	__asm__ __volatile__ ("call *%%eax" : "=a" (ret) : "a" (rbase-pbase+info->entrypoint), "b" (rbase), "c" (&k_export_magic) );
	
	// Error in loading
	if(ret == 1)
	{
		// = Free Executable =
		MM_DeallocRange((void*)rbase, 4);
		return -1;
	}
	
	return 0;
}

/*
int Module_Register(t_unload unload, char *ident, Uint32 *base)
- Called by a module to register itself
*/
int Module_Register(t_unload unload, char *ident, Uint32 *base)
{
	int i;
	
	// Adjust for PIC
	if( (Uint)unload < (Uint)base)
	{
		(t_unload)((Uint)unload) += (Uint)base;
		(char*)((Uint)ident) += (Uint)base;
	}
	
	#if DEBUG
	LogF("Module_Register: (unload=0x%x, ident=%c%c%c%c (0x%x), base=0x%x)\n",
		(Uint)unload, ident[0], ident[1], ident[2], ident[3], (Uint)ident, (Uint)base);
	#endif
	
	// Check Arguments
	if(unload == NULL || ident == NULL) {
		return 0;
	}
	
	// Check if the module already is loaded
	for(i=module_count;i--;)
	{
		if(modules[i].ident[0] == ident[0]
		&& modules[i].ident[1] == ident[1]
		&& modules[i].ident[2] == ident[2]
		&& modules[i].ident[3] == ident[3])
		{
			putsa(CLR_YELLOW|CLR_FLAG, "WARNING: Module '");
			putsa(CLR_YELLOW|CLR_FLAG, ident);
			putsa(CLR_YELLOW|CLR_FLAG, "' is already loaded\n");
			return 0;
		}
	}
	
	// Store Module information
	module_count++;
	modules = realloc(module_count*sizeof(t_module), modules);
	memcpy( modules[module_count-1].ident, ident, 4 );
	modules[module_count-1].unload = unload;
	modules[module_count-1].base = base;
	
	LogF("Module_Register: Registered Module '%c%c%c%c'\n",
		ident[0], ident[1], ident[2], ident[3]);
	
	return 1;
}
