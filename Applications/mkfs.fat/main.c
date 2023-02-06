/*
 AcessOS Version 1
 mkfs.fat
*/
#include "../syscalls.h"
#include "fat.h"

// === PROTOTYPES ===
int DoFormat(char *device);

// === CONSTANTS ===
#define	SECTORS_PER_CLUSTER	8	// 4kB Clusters
const int	cSPC_Cutoffs[] = {
	
};

// === GLOBALS ===

// === CODE ===
int main(int argc, char *argv[])
{
	DoFormat("/Devices/ide/B1");
}

int DoFormat(char *device)
{
	int	fp;
	Uint	length;
	Uint	spc;
	Uint	fatSz;
	fat_bootsect_s	bs;
	
	// Open Device
	fp = open(device, OPEN_FLAG_WRITE);
	if(fp == -1) {
		write(_stdout, 19, "Unable to open disk");
		return -1;
	}
	
	// Get Length
	seek(fp, 0, SEEK_END);
	length = tell(fp);
	seek(fp, 0, SEEK_SET);
	length /= 512;
	
	if(length < 4085) {
		// FAT 12
		spc = SECTORS_PER_CLUSTER;
		fatSz = length / spc;
		fatSz *= 1.5;
	} else if(length < 65525) {
		// FAT 16
		spc = SECTORS_PER_CLUSTER;
		fatSz = length / spc;
		fatSz *= 2;
	} else {
		// FAT 32
		spc = SECTORS_PER_CLUSTER;
		fatSz = length / spc;
		fatSz *= 4;
	}
	
	// Fill Bootsector
	// - Create Jump
	bs.jmp[0] = 0xE9;
	bs.jmp[1] = (sizeof(fat_bootsect_s)-3)&0xFF;
	bs.jmp[2] = (sizeof(fat_bootsect_s)-3)>>8;
	// - Set OEM Name
	bs.oem_name[0] = 'A';	bs.oem_name[1] = 'c';
	bs.oem_name[2] = 'e';	bs.oem_name[3] = 's';
	bs.oem_name[4] = 's';	bs.oem_name[5] = 'O';
	bs.oem_name[6] = 'S';	bs.oem_name[7] = '1';
	// - Set Dims
	bs.bps = 512;
	bs.spc = spc;
	bs.resvSectCount = 2;
	bs.fatCount = 2;
	bs.files_in_root = 0;
	bs.totalSect16 = length;
	bs.mediaDesc = 0xE;
	bs.fatSz16 = fatSz;
}
