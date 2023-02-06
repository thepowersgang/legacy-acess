/**
 Acess Version 1
 \file proc.h
 \brief Process Control
*/
#ifndef PROC_H_
#define PROC_H_

/**
 \struct s_symbol
 \brief Symbol Value
*/
struct s_symbol {
	char	*Name;	//!< Name
	Uint	Value;	//!< Value
	Uint	Flags;	//!< Flags (Relocatable etc.)
};
typedef struct s_symbol	t_symbol;	//!< Symbol Type
/**
 \name Symbol Flags
 \{
*/
#define SYMFLAG_NORELOC	0x1	//!< Indicates that the symbol is relocatable
//!\}

/**
 \struct s_procSection
 \brief Process Section
*/
struct s_procSection {
	Uint32	rva;	//!< Virtal Address
	Uint32	size;	//!< Size
	Uint32	flags;	//!< Flags (Read Only, Code, Allocate)
	Uint32	offset;	//!< File Offset of Data
	Uint32	nameLen;	//!< Length of section name
	char	*name;	//!< Section name
};
typedef struct s_procSection	t_procSection;	//!< Process Section Type

/**
 \struct s_procLibrary
 \brief Process Library
 \todo Expand and make type less simple
*/
struct s_procLibrary {
	char	*name;	//!< Name of library
};
typedef struct s_procLibrary	t_procLibrary;	//!< Process Library Type

/**
 \struct s_executable
 \brief Executable Information Structure
*/
struct s_executable {
	Uint32	entrypoint;	//!<First Instruction
	Uint32	flags;		//!<Flags for executable
	Uint32	sectCount;	//!<Count of sections in memory
	Uint32	PreferedBase;	//!< Prefered Base Address
	Uint32	ActualBase;	//!< True (Loaded) Base Address
	t_procSection	*sects;	//!< List of sections in memory
	t_symbol	*Symbols;	//!< Symbols from the executable image
	 int	SymbolCount;	//!< Symbol Count
	 int	(*relocate)(struct s_executable	*self);	//!< Relocations Callback
};
typedef struct s_executable	t_executable;	//!< Executable Type

/**
 \name Section Flags
 \{
*/
#define	EXE_S_RO		0x01	//!< Read-Only Section
#define	EXE_S_CODE		0x02	//!< Code Section
#define	EXE_S_NOBITS	0x04	//!< Memory Only Section
#define EXE_S_TYPE		0xF0	//!< Section Type Mask
#define	EXE_ST_PLAIN	0x00	//!< Plain Section
#define	EXE_ST_RELOC	0x10	//!< Relocation Section
#define	EXE_ST_STRTAB	0x20	//!< String Table
#define	EXE_ST_SYMTAB	0x30	//!< Symbol Table
//! \}

/**
 \name Executable Flags
 \todo Remove these as they are not used (Superseded by ::RelocateExecutable)
 \{
*/
#define	EXE_P_RELOC		0x03	//!< Relocatable Executable
#define	EXE_P_RELOC_PIC			0x01	//!< PIC Relocations
#define	EXE_P_RELOC_REBASE		0x02	//!< Rebase Relocation
#define	EXE_P_RELOC_UNUSED		0x03	//!< UNUSED
//! \}

/* EXE.C */
/**
 \fn t_executable *LoadExecutable(int fp)
 \brief Loads an executable
 \param fp	File Pointer
 \return Pointer to allocated & filled #t_executable structure
 
 - Determines Executable Type
 - Calls relevant loader
*/
extern t_executable *LoadExecutable(int fp);
/**
 \fn int RelocateExecutable(t_executable *info)
 \brief Relocates an executable
 \param info	Structure returned by ::LoadExecutable
 \return Boolean Failure
 
 - Just calls callback in info
*/
extern int	RelocateExecutable(t_executable *info);
/**
 \fn void FreeExecutable(t_executable *info)
 \brief Frees an executable information structure
 \param info	Structure returned by ::LoadExecutable
*/
extern void	FreeExecutable(t_executable *info);
/**
 \fn char *FindLibrary(char *name)
 \brief Finds a library by searching default paths
 \param name	Name of libary to load (E.g. libc.so)
 \return Path to library or NULL
*/
extern char	*FindLibrary(char *name);

/* DYNLIB.C */
/**
 \fn void *DynLib_Load(char *file, int loadflags)
 \brief Loads a dynamic library into the current process space
 \param file	File path of library
 \param loadflags	Flags to pass to loader
 \return	Pointer to opaque structure representing the loaded library or NULL
*/
extern void	*DynLib_Load(char *file, int loadflags);

/** */

#endif

