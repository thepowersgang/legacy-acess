/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"

// === PROTOTYPES ===
 int	DoRelocate( Uint base, char **envp );
 int	CallUser(Uint entry, int argc, char *argv[], char **envp);
 int	ElfRelocate(void *Base, char *envp[]);

// === Imports ===
extern void	gLinkedBase;
 
// === CODE ===
/**
 \fn int SoMain(Uint base, int argc, char *argv[], char **envp)
 \brief Library entry point
 \note This is the entrypoint for the library
*/
int SoMain(Uint base, int argc, char *argv[], char **envp)
{
	 int	ret;
	 
	// - Assume that the file pointer will be less than 4096
	if(base < 0x1000) {
		SysDebug("ld-acess - SoMain: Passed file pointer %i\n", base);
		SysExit();
		return 0;
	}
	// Check if we are being called directly
	if(base == (Uint)&gLinkedBase) {
		SysDebug("ld-acess compiled for ELF32 only\n");
		SysExit();
		return 0;
	}
	
	// Otherwise do relocations
	ret = DoRelocate( base, envp );
	if( ret == 0 ) {
		SysDebug("ld-acess - SoMain: Relocate failed, base=0x%x\n", base);
		SysExit();
	}
	
	// And call user
	CallUser( ret, argc, argv, envp );
	
	return 0;
}

/**
 \fn int DoRelocate(Uint base, char **envp)
 \brief Relocates an in-memory image
*/
int DoRelocate( Uint base, char **envp )
{
	// Load Executable
	if(*(Uint*)base == (0x7F|('E'<<8)|('L'<<16)|('F'<<24)))
		return ElfRelocate((void*)base, envp);
	
	
	SysDebug("ld-acess - SoMain: Unkown file format '%c%c%c%c'\n",
		*(char*)(base), *(char*)(base+1), *(char*)(base+2), *(char*)(base+3) );
	SysExit();
	return 0;
}

/**
 \fn int CallUser(Uint entry, int argc, char *argv[], char **envp)
*/
int CallUser(Uint entry, int argc, char *argv[], char **envp)
{
	int (*userMain)(int cnt, ...) = (void*)entry;
	SysDebug("Passing Control to user at 0x%x\n", entry);
	return userMain(argc, argv, envp);
}
