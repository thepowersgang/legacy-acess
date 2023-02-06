/*
 AcessOS PCI Dump
*/
#include "../syscalls.h"
#include "header.h"

// === CONSTANTS ===
char data[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// Destination (Broardcast)
	0xb0, 0xc4, 0x20, 0x00, 0x00, 0x10,	// Source (Local MAC Address)
	0x08, 0x06,
	0x00, 0x01,	// HW Type = Ethernet
	0x08, 0x00,	// Proc Type = IP
	0x06,		// HW Address Size = 6
	0x04,		// Proc Address Size = 4
	0x00, 0x01,	// Opcode = Request (#1 - WhoIs)
	0xb0, 0xc4, 0x20, 0x00, 0x00, 0x10,	// Source MAC
	0xc0, 0xa8, 0x01, 0x22,	// Source IP (192.168.1.34)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Destination MAC (0=Unknown)
	0xc0, 0xa8, 0x01, 0x20	// Destination IP (192.168.1.32)
	/*0x00, 0x15, 0xE9, 0x2D, 0x1E, 0x80,
	0xb0, 0xc4, 0x20, 0x00, 0x00, 0x00,
	0x80, 0x00,	// IP
	0xDA, 0x7A,	// DATA
	0x00, 0x00	// CRC32 - Dunno what variant*/
};
#define DATA_LENGTH	(sizeof(data)/sizeof(data[0]))

// === PROTOTYPES ===
int GetHex(char *str);

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	 int	fp;

	write(_stdout, 4, "\x1B[2J");	//Clear Screen	
	printf("NE2000 Tester\n");
	// --- Open PCI Directory
	fp = open("/Devices/ne2000/0", OPEN_FLAG_WRITE, 0);
	if(fp == -1)
	{
		printf("Non-Standard configuration or not running on Acess.\n");
		printf("Quitting - Reason: Unable to open NE2000 Device\n");
		return -1;
	}
	
	printf("Writing Packet...");
	write(fp, DATA_LENGTH, data);
	
	printf(" Done.\n");
	close(fp);
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
