/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"
#include "elf32.h"

#define DEBUG	1

#if !DEBUG
# define	SysDebug(...)	
#endif

// === CONSTANTS ===
#if DEBUG
static const char	*csaDT_NAMES[] = {"DT_NULL", "DT_NEEDED", "DT_PLTRELSZ", "DT_PLTGOT", "DT_HASH", "DT_STRTAB", "DT_SYMTAB", "DT_RELA", "DT_RELASZ", "DT_RELAENT", "DT_STRSZ", "DT_SYMENT", "DT_INIT", "DT_FINI", "DT_SONAME", "DT_RPATH", "DT_SYMBOLIC", "DT_REL", "DT_RELSZ", "DT_RELENT", "DT_PLTREL", "DT_DEBUG", "DT_TEXTREL", "DT_JMPREL"};
static const char	*csaR_NAMES[] = {"R_386_NONE", "R_386_32", "R_386_PC32", "R_386_GOT32", "R_386_PLT32", "R_386_COPY", "R_386_GLOB_DAT", "R_386_JMP_SLOT", "R_386_RELATIVE", "R_386_GOTOFF", "R_386_GOTPC", "R_386_LAST"};
#endif

// === PROTOTYPES ===
void elf_doRelocate(Uint r_info, Uint32 *ptr, Uint32 addend, Elf32_Sym *symtab, Uint base);

// === CODE ===
/**
 \fn int ElfRelocate(void *SectionHeader, int iSectionCount, void *Base)
 \brief Relocates a loaded ELF Executable
*/
int ElfRelocate(void *Base, char *envp[])
{
	 int	i, j;	// Counters
	char	*strtab = NULL;	// String Table Pointer
	char	*shstrtab;	// String Table Pointer
	char	*dynstrtab = NULL;	// .dynamic String Table
	Elf32_Dyn	*dynamicTab = NULL;	// Dynamic Table Pointer
	char	*libPath;
	//void	*hLibrary;
	Uint	iRealBase = -1;
	Uint	iBaseDiff;
	 int	iSectionCount;
	Elf32_Ehdr	*hdr = Base;
	Elf32_Shent	*shtab;
	Elf32_Sym	*symtab;
	Elf32_Sym	*dynsymtab;
	 int	iSymCount;
	Elf32_Rel	*rel = NULL;
	Elf32_Rela	*rela = NULL;
	Uint32	*pltgot = NULL;
	void	*plt = NULL;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	
	SysDebug("elf_relocate: (Base=0x%x)\n", Base);
	
	// Get Section Header
	shtab = Base + hdr->shoff;
	shstrtab = Base + shtab[hdr->shstrindex].offset;
	iSectionCount = hdr->shentcount;
	
	// - Parse Section Table
	//  > Determine Linked base
	//  > Find Symbol Table
	for(i=0;i<iSectionCount;i++)
	{
		SysDebug(" elf_relocate: %i '%s' 0x%x : 0x%x\n", i, shstrtab + shtab[i].name, shtab[i].address, shtab[i].flags);
		// Determine linked base address
		if(iRealBase > shtab[i].address && shtab[i].flags & SHF_ALLOC)
			iRealBase = shtab[i].address;
		// Locate Symbol Table
		if(shtab[i].type == SHT_SYMTAB) {
			symtab = Base + shtab[i].offset;
			iSymCount = shtab[i].size / sizeof(*symtab);
			SysDebug(" elf_relocate: Symbol Table '%s' at 0x%x\n", shstrtab + shtab[i].name, symtab);
		}
		// Locate String Table
		if(shtab[i].type == SHT_STRTAB) {
			strtab = (char*)( Base + shtab[i].offset );
			SysDebug(" elf_relocate: String Table '%s' at 0x%x\n", shstrtab + shtab[i].name, strtab);
		}
	}
	
	iRealBase &= ~0xFFF;

	SysDebug(" elf_relocate: True Base = 0x%x, Compiled Base = 0x%x\n", Base, iRealBase);
	
	// Adjust "Real" Base
	iBaseDiff = (Uint)Base - iRealBase;

	// Alter Symbols to true base
	for(i=0;i<iSymCount;i++)
	{
		symtab[i].value += iBaseDiff;
		symtab[i].nameOfs += (Uint)strtab;
		//SysDebug("elf_relocate: Sym '%s' = 0x%x pre relocations\n", symtab[i].name, symtab[i].value);
	}
	
	// === Get Symbol table and String Table ===
	for(i=0;i<iSectionCount;i++)
	{
		if(shtab[i].type == SHT_DYNAMIC)
		{
			dynamicTab = (void*)( (Uint)Base + shtab[i].offset );
			for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
			{
				switch(dynamicTab[j].d_tag)
				{
				// --- Symbol Table ---
				case DT_SYMTAB:
					SysDebug(" elf_relocate: Symbol Table 0x%x\n", dynamicTab[j].d_val);
					dynsymtab = (void*)(iBaseDiff + dynamicTab[j].d_val);
					break;
				// --- String Table ---
				case DT_STRTAB:
					SysDebug(" elf_relocate: String Table 0x%x\n", dynamicTab[j].d_val);
					dynstrtab = (void*)(iBaseDiff + dynamicTab[j].d_val);
					break; 
				}
			}
		}
	}

	for(i=0;i<iSectionCount;i++)
	{
		if((Uint)Base + shtab[i].offset == (Uint)dynsymtab) {
			iSymCount = shtab[i].size / sizeof(*symtab);
			break;
		}
	}

	// Alter Symbols to true base
	for(i=0;i<iSymCount;i++)
	{
		dynsymtab[i].value += iBaseDiff;
		dynsymtab[i].nameOfs += (Uint)dynstrtab;
		SysDebug("elf_relocate: Sym '%s' = 0x%x (relocated)\n", dynsymtab[i].name, dynsymtab[i].value);
	}
	
	// === Add to loaded list (can be imported now) ===
	AddLoaded( (Uint)Base );

	// === Parse Relocation Data ===
	for(i=0;i<iSectionCount;i++)
	{
		// Dynamic Section
		if(shtab[i].type == SHT_DYNAMIC || shtab[i].type == SHT_DYNSYM
		|| shtab[i].type == SHT_RELA || shtab[i].type == SHT_REL || shtab[i].type == SHT_HASH )
		{
			dynamicTab = (void*)( (Uint)Base + shtab[i].offset );
			SysDebug(" elf_relocate: dynamicTab = 0x%x\n", dynamicTab);
			for( j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
			{
				switch(dynamicTab[j].d_tag)
				{
				// --- Shared Library Name ---
				case DT_SONAME:
					SysDebug(" elf_relocate: .so Name '%s'\n", dynstrtab+dynamicTab[j].d_val);
					break;
				// --- Needed Library ---
				case DT_NEEDED:
					libPath = dynstrtab + dynamicTab[j].d_val;
					SysDebug(" elf_relocate: Required Library '%s'\n", libPath);
					if(LoadLibrary(libPath, envp) == 0) {
						SysDebug(" elf_relocate: Unable to load '%s'\n", libPath);
						return 0;
					}
					break;
				// --- PLT/GOT ---
				case DT_PLTGOT:	pltgot = (void*)iBaseDiff+(dynamicTab[j].d_val);	break;
				case DT_JMPREL:	plt = (void*)(iBaseDiff+dynamicTab[j].d_val);	break;
				case DT_PLTREL:	pltType = dynamicTab[j].d_val;	break;
				case DT_PLTRELSZ:	pltSz = dynamicTab[j].d_val;	break;
				
				// --- Relocation ---
				case DT_REL:	rel = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
				case DT_RELSZ:	relSz = dynamicTab[j].d_val;	break;
				case DT_RELENT:	relEntSz = dynamicTab[j].d_val;	break;
				case DT_RELA:	rela = (void*)(iBaseDiff + dynamicTab[j].d_val);	break;
				case DT_RELASZ:	relaSz = dynamicTab[j].d_val;	break;
				case DT_RELAENT:	relaEntSz = dynamicTab[j].d_val;	break;
				
				// --- Symbol Table ---
				case DT_SYMTAB:
				// --- String Table ---
				case DT_STRTAB:
					break;
				
				// --- Unknown ---
				default:
					if(dynamicTab[j].d_tag > DT_JMPREL)	continue;
					SysDebug(" elf_relocate: %i-%i = %s,0x%x\n",
						i,j, csaDT_NAMES[dynamicTab[j].d_tag],dynamicTab[j].d_val);
					break;
				}
			}
		}
	}
	SysDebug(" elf_relocate: Beginning Relocation\n");
	
	// Parse Relocation Entries
	if(rel && relSz)
	{
		Uint32	*ptr;
		SysDebug(" elf_relocate: rel=0x%x, relSz=0x%x, relEntSz=0x%x\n", rel, relSz, relEntSz);
		j = relSz / relEntSz;
		for( i = 0; i < j; i++ )
		{
			//SysDebug("  Rel %i: 0x%x+0x%x\n", i, iBaseDiff, rel[i].r_offset);
			ptr = (void*)(iBaseDiff + rel[i].r_offset);
			elf_doRelocate(rel[i].r_info, ptr, *ptr, dynsymtab, iBaseDiff);
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		Uint32	*ptr;
		SysDebug(" elf_relocate: rela=0x%x, relaSz=0x%x, relaEntSz=0x%x\n", rela, relaSz, relaEntSz);
		j = relaSz / relaEntSz;
		for( i = 0; i < j; i++ )
		{
			ptr = (void*)(iBaseDiff + rela[i].r_offset);
			elf_doRelocate(rel[i].r_info, ptr, rela[i].r_addend, dynsymtab, iBaseDiff);
		}
	}
	
	// === Process PLT (Procedure Linkage Table) ===
	if(plt && pltSz)
	{
		Uint32	*ptr;
		SysDebug(" elf_relocate: Relocate PLT, plt=0x%x\n", plt);
		if(pltType == DT_REL)
		{
			Elf32_Rel	*pltRel = plt;
			j = pltSz / sizeof(Elf32_Rel);
			SysDebug(" elf_relocate: PLT Reloc Type = Rel, %i entries\n", j);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				elf_doRelocate(pltRel[i].r_info, ptr, *ptr, dynsymtab, iBaseDiff);
			}
		}
		else
		{
			Elf32_Rela	*pltRela = plt;
			SysDebug(" elf_relocate: PLT Reloc Type = Rela\n");
			j = pltSz / sizeof(Elf32_Rela);
			for(i=0;i<j;i++)
			{
				ptr = (void*)(iRealBase + pltRela[i].r_offset);
				elf_doRelocate(pltRela[i].r_info, ptr, pltRela[i].r_addend, dynsymtab, iRealBase);
			}
		}
	}
	
	if(pltgot)
	{
		SysDebug(" elf_relocate: GOT = 0x%x\n", pltgot);
		for(i=0;i<4;i++)
		{
			SysDebug(" elf_relocate: GOT[%i] = 0x%x\n", i, pltgot[i]);
		}
	}
	
	return hdr->entrypoint;
}

void elf_doRelocate(Uint r_info, Uint32 *ptr, Uint32 addend, Elf32_Sym *symtab, Uint base)
{
	 int	type = ELF32_R_TYPE(r_info);
	 int	sym = ELF32_R_SYM(r_info);
	Uint32	val;
	switch( type )
	{
	// Standard 32 Bit Relocation (S+A)
	case R_386_32:
		val = GetSymbol( symtab[sym].name );
		SysDebug(" elf_doRelocate: R_386_32 *0x%x += 0x%x('%s')\n",
			ptr, val, symtab[sym].name);
		//*ptr = addend + val - ((Uint)ptr-info->preferedBase-2);
		*ptr = val + addend;
		break;
		
	// 32 Bit Relocation wrt. Offset (S+A-P)
	case R_386_PC32:
		val = GetSymbol( symtab[sym].name );
		SysDebug(" elf_doRelocate: R_386_PC32 *0x%x = 0x%x + 0x%x('%s') - 0x%x\n",
			ptr, *ptr, val, symtab[sym].name, ((Uint)ptr - base) );
		*ptr = val + addend - ((Uint)ptr - base);
		break;

	// Absolute Value of a symbol (S)
	case R_386_GLOB_DAT:
	case R_386_JMP_SLOT:
		SysDebug(" elf_doRelocate: #%i: '%s'\n", sym, symtab[sym].name);
		val = GetSymbol( symtab[sym].name );
		SysDebug(" elf_doRelocate: %s *0x%x = 0x%x\n", csaR_NAMES[type], ptr, val);
		*ptr = val;
		break;

	// Base Address (B+A)
	case R_386_RELATIVE:
		SysDebug(" elf_doRelocate: R_386_RELATIVE *0x%x = 0x%x + 0x%x\n", ptr, 0, addend);
		*ptr = base + addend;
		break;
		
	default:
		SysDebug(" elf_doRelocate: Rel 0x%x: 0x%x,%s\n", ptr, sym, csaR_NAMES[type]);
		break;
	}
	
}

int ElfGetSymbol(Uint Base, char *name, Uint *ret)
{
	Elf32_Ehdr	*hdr = (void*)Base;
	Elf32_Shent	*shtab;
	Elf32_Sym	*symtab;
	 int	iSymCount = 0;
	 int	i;
	
	shtab = (void*)( Base + hdr->shoff );

	//  > Find Symbol Table
	for(i=0;i<hdr->shentcount;i++)
	{
		// Locate Symbol Table
		if(shtab[i].type == SHT_SYMTAB) {
			symtab = (void*)( Base + shtab[i].offset );
			iSymCount = shtab[i].size / sizeof(*symtab);
			break;
		}
	}
	if(i == hdr->shentcount)	return 0;
	
	//SysDebug("%i Symbols at 0x%x\n", iSymCount, symtab);

	// Scan Symbol Table
	for(i=0;i<iSymCount;i++)
	{
		if(symtab[i].shndx == SHN_UNDEF)	continue;
		if(strcmp(symtab[i].name, name) == 0) {
			*ret = symtab[i].value;
			return 1;
		}
	}
	
	return 0;
}
