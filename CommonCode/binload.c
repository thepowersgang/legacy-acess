/*
Acess v0.1
Binary Loader Code
*/
#include <system.h>
#include <acess.h>
#include <binload.h>

#define	BIN_DEBUG	0
#define	DEBUG	((BIN_DEBUG)|(ACESS_DEBUG))

#include <dbg_defs.h>

// === CONSTANTS ===
#define BIN_LOWEST	0x100000		// 1MiB
#define BIN_GRANUALITY	0x10000		// 64KiB
#define BIN_HIGHEST	(0xC0000000-BIN_GRANUALITY)		// 3GiB - 1 Granuality (Just below the kernel)

// === GLOBALS ===
tLoadedBin	*glLoadedBinaries = NULL;
char	**gsaRegInterps = NULL;
 int	giRegInterps = 0;

// === IMPORTS ===
extern tLoadedBin	*Elf_Load(int fp);
extern void 	MM_SetCOW(Uint addr);

// === PROTOTYPES ===
Uint Binary_Load(char *file, Uint *entryPoint);
tLoadedBin *Binary_GetInfo(char *truePath);
Uint Binary_MapIn(tLoadedBin *binary);
tLoadedBin *Binary_DoLoad(char *truePath);
 
// === FUNCTIONS ===
Uint Binary_Load(char *file, Uint *entryPoint)
{
	char	*sTruePath;
	tLoadedBin	*pBinary;
	Uint	base = -1;

	DEBUGS("Binary_Load: (file='%s')\n", file);
	
	// Sanity Check Argument
	if(file == NULL)	return 0;

	// Get True File Path
	sTruePath = VFS_GetTruePath(file);
	
	if(sTruePath == NULL) {
		warning("Binary_Load - '%s' does not exist.\n", file);
		return 0;
	}
	
	DEBUGS(" Binary_Load: sTruePath = '%s'\n", sTruePath);

	// Check if the binary has already been loaded
	if( !(pBinary = Binary_GetInfo(sTruePath)) )
		pBinary = Binary_DoLoad(sTruePath);	// Else load it
	
	// Clean Up
	free(sTruePath);
	
	// Error Check
	if(pBinary == NULL)
		return 0;
	
	// Map into process space
	base = Binary_MapIn(pBinary);	// If so then map it in
	
	// Check for errors
	if(base == 0)	return 0;
	
	// Interpret
	if(pBinary->Interpreter)
	{
		Uint start;
		if( Binary_Load(pBinary->Interpreter, &start) == 0 )
			return 0;
		*entryPoint = start;
	}
	else
		*entryPoint = pBinary->EntryPoint - pBinary->Base + base;
	
	// Return
	DEBUGS("Binary_Load: RETURN 0x%x, *entryPoint = 0x%x\n", base, *entryPoint);
	//if(pBinary->Interpreter)
		return base;	// Pass the base as an argument to the user if there is an interpreter
	//else
	//	return -1;	// -1 Indicates that there is to be no extra argument added passed
}

/**
 \fn tLoadedBin *Binary_GetInfo(char *truePath)
 \brief Finds a matching binary entry
 \param truePath	File Identifier (True path name)
*/
tLoadedBin *Binary_GetInfo(char *truePath)
{
	tLoadedBin	*pBinary;
	pBinary = glLoadedBinaries;
	while(pBinary)
	{
		if(strcmp(pBinary->TruePath, truePath) == 0)
			return pBinary;
		pBinary = pBinary->Next;
	}
	return NULL;
}

/**
 \fn Uint Binary_MapIn(tLoadedBin *binary)
 \brief Maps an already-loaded binary into an address space.
 \param binary	Pointer to globally stored data.
*/
Uint Binary_MapIn(tLoadedBin *binary)
{
	Uint	base;
	Uint	addr;
	 int	i;
	
	// Reference Executable (Makes sure that it isn't unloaded)
	binary->ReferenceCount ++;
	
	// Get Binary Base
	base = binary->Base;
	
	// Check if base is free
	if(base != 0)
	{
		for(i=0;i<binary->PageCount;i++)
		{
			if( MM_IsValid( binary->Pages[i].Virtual & ~0xFFF ) ) {
				base = 0;
				LogF(" Binary_MapIn: Address 0x%x is taken\n", binary->Pages[i].Virtual & ~0xFFF);
				break;
			}
		}
	}
	
	// Check if the executable has no base or it is not free
	if(base == 0)
	{
		// If so, give it a base
		base = BIN_HIGHEST;
		while(base >= BIN_LOWEST)
		{
			for(i=0;i<binary->PageCount;i++)
			{
				addr = binary->Pages[i].Virtual & ~0xFFF;
				addr -= binary->Base;
				addr += base;
				if( MM_IsValid( addr ) )	break;
			}
			// If space was found, break
			if(i == binary->PageCount)		break;
			// Else decrement pointer and try again
			base -= BIN_GRANUALITY;
		}
	}
	
	// Error Check
	if(base < BIN_LOWEST) {
		warning("[BIN ] Executable '%s' cannot be loaded into PID %i, no space\n",
			binary->TruePath, proc_pid);
		return 0;
	}
	
	// Map Executable In
	for(i=0;i<binary->PageCount;i++)
	{
		addr = binary->Pages[i].Virtual & ~0xFFF;
		addr -= binary->Base;
		addr += base;
		DEBUGS(" Binary_DoLoad: %i - 0x%x to 0x%x\n", i, addr, binary->Pages[i].Physical);
		mm_map( addr, (Uint) (binary->Pages[i].Physical) );
		if( binary->Pages[i].Physical & 1)	// Read-Only
			mm_setro( addr, 1 );
		else
			MM_SetCOW( addr );
	}
	
	return base;
}

/**
 \fn tLoadedBin *Binary_DoLoad(char *truePath)
 \brief Loads a binary file into memory
 \param truePath	Absolute filename of binary
*/
tLoadedBin *Binary_DoLoad(char *truePath)
{
	tLoadedBin	*pBinary;
	 int	fp, i;
	Uint	ident, addr;
	
	DEBUGS("Binary_DoLoad: (truePath='%s')\n", truePath);
	
	// Open File
	fp = vfs_open(truePath, VFS_OPENFLAG_READ);
	
	// Read File Type
	vfs_read(fp, 4, &ident);
	vfs_seek(fp, 0, 1);
	
	// Determine File Type
	if( ident == (0x7F|('E'<<8)|('L'<<16)|('F'<<24)) )
		pBinary = Elf_Load(fp);
	//else if( ident == ('A')+('X'<<8)+('E'<<16) )
	//	pBinary = Axe_Load(fp);
	else {
		warning("[BIN ] '%s' is an unknown file type. (%c%c%c%c)\n", truePath,
			ident&0xFF, ident>>8, ident>>16, ident>>24);
		return 0;
	}
	
	// Error Check
	if(pBinary == NULL)	return 0;
	
	// Initialise Structure
	pBinary->ReferenceCount = 0;
	pBinary->TruePath = malloc( strlen(truePath) + 1 );
	strcpy(pBinary->TruePath, truePath);
	
	// Debug Information
	DEBUGS(" Binary_DoLoad: Interpreter: '%s'\n", pBinary->Interpreter);
	DEBUGS(" Binary_DoLoad: Base: 0x%x, Entry: 0x%x\n", pBinary->Base, pBinary->EntryPoint);
	DEBUGS(" Binary_DoLoad: PageCount: %i\n", pBinary->PageCount);
	
	// Read Data
	for(i=0;i<pBinary->PageCount;i++)
	{
		addr = (Uint)MM_AllocPhys();
		ident = MM_MapTemp( addr );
		if(pBinary->Pages[i].Physical == -1) {
			DEBUGS(" Binary_DoLoad: %i - ZERO\n", i);
			memsetda( (void*)ident, 0, 1024 );
		} else {
			DEBUGS(" Binary_DoLoad: %i - 0x%x\n", i, pBinary->Pages[i].Physical);
			vfs_seek( fp, pBinary->Pages[i].Physical, 1);
			vfs_read( fp, 0x1000, (void*)ident );
		}
		pBinary->Pages[i].Physical = addr;
		MM_FreeTemp( ident );
	}
	DEBUGS(" Binary_DoLoad: Page Count: %i\n", pBinary->PageCount);
	
	// Close File
	vfs_close(fp);
	
	// Add to the list
	LOCK();
	pBinary->Next = glLoadedBinaries;
	glLoadedBinaries = pBinary;
	UNLOCK();
	
	// Return
	return pBinary;
}

/**
 \fn char *Binary_RegInterp(char *path)
 \brief Registers an Interpreter
 \param path	Path to interpreter provided by executable
*/
char *Binary_RegInterp(char *path)
{
	 int	i;
	// NULL Check Argument
	if(path == NULL)	return NULL;
	// NULL Check the array
	if(gsaRegInterps == NULL)
	{
		giRegInterps = 1;
		gsaRegInterps = malloc( sizeof(char*) );
		gsaRegInterps[0] = malloc( strlen(path) );
		strcpy(gsaRegInterps[0], path);
		return gsaRegInterps[0];
	}
	
	// Scan Array
	for( i = 0; i < giRegInterps; i++ )
	{
		if(strcmp(gsaRegInterps[i], path) == 0)
			return gsaRegInterps[i];
	}
	
	// Interpreter is not in list
	giRegInterps ++;
	gsaRegInterps = malloc( sizeof(char*)*giRegInterps );
	gsaRegInterps[i] = malloc( strlen(path) );
	strcpy(gsaRegInterps[i], path);
	return gsaRegInterps[i];
}
