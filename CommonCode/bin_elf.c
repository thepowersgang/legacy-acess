/*
Acess v0.1
ELF Executable Loader Code
*/
#include <acess.h>
#include <binload.h>
#include <bin_elf.h>

// === CODE ===
tLoadedBin *Elf_Load(int fp)
{
	tLoadedBin	*ret;
	Elf32_Ehdr	hdr;
	Elf32_Phdr	*phtab;
	 int	i, j, k;
	 int	iPageCount;
	 int	count;
	
	// Read ELF Header
	vfs_read(fp, sizeof(hdr), &hdr);
	
	// Check the file type
	if(hdr.ident[0] != 0x7F || hdr.ident[1] != 'E' || hdr.ident[2] != 'L' || hdr.ident[3] != 'F')
		return NULL;
	
	// Check for a program header
	if(hdr.phoff == 0)
		return NULL;
	
	// Count Pages
	iPageCount = 0;
	phtab = malloc(sizeof(Elf32_Phdr)*hdr.phentcount);
	vfs_read(fp, sizeof(Elf32_Phdr)*hdr.phentcount, phtab);
	vfs_seek(fp, hdr.phoff, 1);
	for( i = 0; i < hdr.phentcount; i++ )
	{
		// Ignore Non-LOAD types
		if(phtab[i].Type != PT_LOAD)
			continue;
		iPageCount += (phtab[i].MemSize + 0xFFF) >> 12;
	}
	
	// Allocate Information Structure
	ret = malloc( sizeof(tLoadedBin) + 2*sizeof(Uint)*iPageCount );
	// Fill Info Struct
	ret->EntryPoint = hdr.entrypoint;
	ret->Base = -1;		// Set Base to maximum value
	ret->PageCount = iPageCount;
	ret->Interpreter = NULL;
	
	// Load Pages
	j = 0;
	vfs_seek(fp, hdr.phoff, 1);
	for( i = 0; i < hdr.phentcount; i++ )
	{
		// Get Interpreter Name
		if( phtab[i].Type == PT_INTERP )
		{
			char *tmp;
			if(ret->Interpreter)	continue;
			tmp = malloc(phtab[i].FileSize);
			vfs_seek(fp, phtab[i].Offset, 1);
			vfs_read(fp, phtab[i].FileSize, tmp);
			ret->Interpreter = Binary_RegInterp(tmp);
			free(tmp);
			continue;
		}
		// Ignore non-LOAD types
		if(phtab[i].Type != PT_LOAD)	continue;
		
		// Find Base
		if(phtab[i].VAddr < ret->Base)	ret->Base = phtab[i].VAddr;
		
		#if 1
		// Get Pages
		count = (phtab[i].FileSize + 0xFFF) >> 12;
		for(k=0;k<count;k++)
		{
			ret->Pages[j+k].Virtual = phtab[i].VAddr + (k<<12);
			ret->Pages[j+k].Physical = phtab[i].Offset + (k<<12);	// Store the offset in the physical address
		}
		count = (phtab[i].MemSize + 0xFFF) >> 12;
		for(;k<count;k++)
		{
			ret->Pages[j+k].Virtual = phtab[i].VAddr + (k<<12);
			ret->Pages[j+k].Physical = -1;	// Store the offset in the physical address
		}
		j += count;
		#else
		count = (phtab[i].MemSize + 0xFFF) >> 12;
		for(k=0;k<count;k++)
		{
			ret->Pages[j+k].Virtual = phtab[i].VAddr + (k<<12);
			ret->Pages[j+k].Physical = -1;	// Store the offset in the physical address
		}
		j += count;
		#endif
	}
	
	// Calculate space taken by binary
	//ret->PageSpace = (ret->PageSpace+0xFFF) >> 12;
	
	// Clean Up
	free(phtab);
	// Return
	return ret;
}
