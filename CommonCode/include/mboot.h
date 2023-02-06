/*
AcessOS/AcessBasic v0.1
Multiboot Information
HEADER
*/
#ifndef _MBOOT_H
#define _MBOOT_H

#define MULTIBOOT_MAGIC	0x2BADB002

/**
 \struct mboot_info_s
 \brief Multiboot information structure
*/
struct mboot_info_s {
	Uint8	flags;		//!< Multiboot Flags
	Uint32	mem_lower;	//!< Lower memory
	Uint32	mem_upper;	//!< Upper memory
	Uint8	boot_device[4];	//!< Boot Device - array in order of part3,part2,part1,disk
	Uint32	cmdline;	//!< Pointer to command line arguments provided to kernel
	Uint32	mods_count;	//!< Count of modules loaded
	Uint32	mods_start;	//!< Address of first module
	Uint32	elf_shdr_num;	//!< ELF Section Count
	Uint32	elf_shdr_size;	//!< ELF Section Header Size
	Uint32	elf_shdr_addr;	//!< ELF Section Header Adddress
	Uint32	elf_shdr_shndx;	//!< ELF String Table Index
};

/**
 \struct mboot_mod_s
 \brief Multiboot module information
*/
struct mboot_mod_s {
	Uint32	mod_start;	//!< Address of place where the module code is loaded
	Uint32	mod_end;	//!< Address of end of module
	Uint32	string;		//!< String provided, e.g. module name
	Uint8	reserved;	//!< Reserved (Set to 0)
};

typedef struct mboot_info_s mboot_info;
typedef struct mboot_mod_s mboot_mod;

#endif
