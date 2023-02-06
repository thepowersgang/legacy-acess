/*
 AcessOS PCI Dump
*/
#include "../syscalls.h"
#include "header.h"

// === CONSTANTS ===
const struct {
	 int	ID;
	char *Name;
} csaVENDORS[] = {
	{0x8086, "Intel"},
	{0x10EC, "Realtek"}
};
#define VENDOR_COUNT	(sizeof(csaVENDORS)/sizeof(csaVENDORS[0]))

// === PROTOTYPES ===
int GetHex(char *str);
char *GetVendorName(Uint16 id);

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	 int	dp, fp;
	char	tmpPath[256+13] = "/Devices/pci/";
	char	fileName[256];
	Uint16	vendor, device;
	Uint32	tmp32;
	t_fstat	stats;
	
	printf("PCI Bus Dump\n");
	// --- Open PCI Directory
	dp = open("/Devices/pci", /*OPEN_FLAG_READ|*/OPEN_FLAG_EXEC, 0);
	if(dp == -1)
	{
		printf("Non-Standard configuration or not running on Acess.\n");
		printf("Quitting - Reason: Unable to open PCI driver.\n");
		return -1;
	}
	
	// Get File Stats
	if( stat(dp, &stats) == -1 )
	{
		close(dp);
		printf("stat Failed, Bad File Descriptor\n");
		return -2;
	}
	// Check if it's a directory
	if(!(stats.st_mode & S_IFDIR))
	{
		close(dp);
		printf("Unable to open directory `%s', Not a directory", tmpPath);
		return -3;
	}
	
	// --- List Contents
	while( (fp = readdir(dp, fileName)) )
	{
		if(fp < 0) {
			if(fp == -3)	printf("Invalid Permissions to traverse directory\n");
			break;
		}
		
		//sprintf("%s\n", fileName);
		printf("Bus %c%c, Index %c%c, Fcn %c: ", fileName[0],fileName[1],
			fileName[3],fileName[4], fileName[6]);
		
		// Create File Path
		strcpy((char*)(tmpPath+13), fileName);
		//printf("%s\n", tmpPath);
		// Open File
		fp = open(tmpPath, OPEN_FLAG_READ, 0);
		//printf("fp = %i\n", fp);
		if(fp == -1)	continue;
		
		read(fp, 2, &vendor);	read(fp, 2, &device);
		printf(" Vendor 0x%x (%s), Device 0x%x\n", vendor, GetVendorName(vendor), device);
		//printf(" Vendor 0x%x, Device 0x%x\n", vendor, device);
		
		// Read File
		seek(fp, 0x10, 1);	//SEEK_SET
		printf("Base Address Registers (BARs):\n");
		read(fp, 4, &tmp32);	printf(" 0x%x", tmp32);
		read(fp, 4, &tmp32);	printf(" 0x%x", tmp32);
		read(fp, 4, &tmp32);	printf(" 0x%x\n", tmp32);
		read(fp, 4, &tmp32);	printf(" 0x%x", tmp32);
		read(fp, 4, &tmp32);	printf(" 0x%x", tmp32);
		read(fp, 4, &tmp32);	printf(" 0x%x\n", tmp32);
		printf("\n");
		
		// Close File
		close(fp);
	}
	close(dp);
	return 0;
}

int GetHex(char *str)
{
	int ret = 0;
	for(;;)
	{
		if(*str < '0')	break;
		if(*str > 'f')	break;
		if(*str > 'F' && *str < 'a')	break;
		if(*str > '9' && *str < 'A')	break;
		
		ret <<= 4;
		if(*str >= 'a')
			ret += *str-'a'+10;
		else if(*str >= 'A')
			ret += *str-'A'+10;
		else
			ret += *str-'0';
		str++;
	}
	return ret;
}

char *GetVendorName(Uint16 id)
{
	int i;
	for(i=0;i<VENDOR_COUNT;i++)
	{
		if( csaVENDORS[i].ID == id )
			return csaVENDORS[i].Name;
	}
	return "Unknown";
}
