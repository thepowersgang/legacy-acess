/*
AcessBasic Native Executable
 .AXE Format
 
 Header Definition
*/

#ifndef _AXE_H
#define _AXE_H

typedef struct {
	unsigned int	length;
	unsigned int	loadto;
	unsigned int	entry;
	unsigned int	maxmem;
	unsigned int	flags;
	unsigned int	checksum;
} tAxeHeader;

#endif
