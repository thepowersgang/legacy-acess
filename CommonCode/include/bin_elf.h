/*
Acess v0.1
ELF Executable Loader Code
*/
#ifndef _BIN_ELF_H
#define _BIN_ELF_H

/**
 \struct elf_header_s
 \brief ELF File Header
*/
struct sElf32_Ehdr {
	char	ident[16];	//!< Identifier Bytes
	Uint16	filetype;	//!< File Type
	Uint16	machine;	//!< Machine / Arch
	Uint32	version;	//!< Version (File?)
	Uint32	entrypoint;	//!< Entry Point
	Uint32	phoff;	//!< Program Header Offset
	Uint32	shoff;	//!< Section Header Offset
	Uint32	flags;	//!< Flags
	Uint16	headersize;	//!< Header Size
	Uint16	phentsize;	//!< Program Header Entry Size
	Uint16	phentcount;	//!< Program Header Entry Count
	Uint16	shentsize;	//!< Section Header Entry Size
	Uint16	shentcount;	//!< Section Header Entry Count
	Uint16	shstrindex;	//!< Section Header String Table Index
};

struct sElf32_Phdr {
	Uint32	Type;
	Uint	Offset;
	Uint	VAddr;
	Uint	PAddr;
	Uint32	FileSize;
	Uint32	MemSize;
	Uint32	Flags;
	Uint32	Align;
};

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

typedef struct sElf32_Ehdr	Elf32_Ehdr;
typedef struct sElf32_Phdr	Elf32_Phdr;

#endif
