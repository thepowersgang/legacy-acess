/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"

// === PROTOTYPES ===
 int	IsLoaded(Uint base);
 int	GetSymbolFromBase(Uint base, char *name, Uint *ret);

// === GLOABLS ===
Uint	gLoadedLibraries[1024];
//tLoadLib	*gpLoadedLibraries = NULL;

// === CODE ===
int	LoadLibrary(char *filename, char **envp)
{
	char	sTmpName[1024] = "/Mount/hda1/Acess/Libs/";
	Uint	iArg;
	void	(*fEntry)(int, int, char *[], char**);
	
	SysDebug("LoadLibrary: (filename='%s', envp=0x%x)\n", filename, envp);
	
	// Create Temp Name
	strcpy(&sTmpName[12], filename);
	SysDebug(" LoadLibrary: sTmpName='%s'\n", sTmpName);
	
	// Load Library
	iArg = SysLoadBin(sTmpName, (Uint*)&fEntry);
	if(iArg == 0) {
		SysDebug("LoadLibrary: RETURN 0\n");
		return 0;
	}
	
	SysDebug("LoadLibrary: iArg=0x%x, iEntry=0x%x\n", iArg, fEntry);
	// Call Entrypoint
	fEntry(iArg, 0, NULL, envp);
	
	// Load Symbols
	if( !IsLoaded(iArg) )
		DoRelocate( iArg, envp );
	
	SysDebug("LoadLibrary: RETURN 1\n");
	return 1;
}

int IsLoaded(Uint base)
{
	int i;
	for(i=0;i<sizeof(gLoadedLibraries)/sizeof(gLoadedLibraries[0]);i++)
	{
		if(gLoadedLibraries[i] == base)	return i;
		if(gLoadedLibraries[i] == 0)	continue;
	}
	return 0;
}

void AddLoaded(Uint base)
{
	int i;
	for(i=0;i<sizeof(gLoadedLibraries)/sizeof(gLoadedLibraries[0]);i++)
	{
		if(gLoadedLibraries[i] != 0)	continue;
		gLoadedLibraries[i] = base;
		SysDebug("0x%x loaded as %i\n", base, i);
		return;
	}
}
/**
 \fn Uint GetSymbol(char *name)
 \brief Gets a symbol value from a loaded library
*/
Uint GetSymbol(char *name)
{
	 int	i;
	Uint	ret;
	for(i=0;i<sizeof(gLoadedLibraries)/sizeof(gLoadedLibraries[0]);i++)
	{
		if(gLoadedLibraries[i] == 0)	continue;
//		SysDebug(" GetSymbol: Trying 0x%x\n", gLoadedLibraries[i]);
		if(GetSymbolFromBase(gLoadedLibraries[i], name, &ret))	return ret;
	}
	return 0;
}

/**
 \fn int GetSymbolFromBase(Uint base, char *name, Uint *ret)
 \breif Gets a symbol from a specified library
*/
int GetSymbolFromBase(Uint base, char *name, Uint *ret)
{
	Uint ident;
	ident = *(Uint*)base;
	if(ident == (0x7F|('E'<<8)|('L'<<16)|('F'<<24)))
		return ElfGetSymbol(base, name, ret);
	return 0;
}

