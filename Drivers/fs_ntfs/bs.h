typedef unsigned long long	Uint64;
typedef unsigned long	Uint32;
typedef unsigned short	Uint16;
typedef unsigned char	Uint8;

struct {
	//0x00
	char	jmp[3];
	char	oemid[8];
	Uint16	bps;
	Uint8	spc;
	Uint16	resSect;
	//0x10
	char	res1[3];	//00 00 00
	Uint16	unused1;
	char	mediaDesc;	//0xF8
	Uint16	res2;	//00
	Uint16	spt;
	Uint16	numHead;
	Uint32	hiddenSectors;
	//0x20
	Uint32	unused2;
	Uint32	unused3;
	Uint64	totalSectors;
	//0x30
	Uint64	mftOff;		//Cluster
	Uint64	mtfMirrOff;	//Cluster
	//0x40
	Uint32	fileRecSize;
	Uint32	indexBlockSize;
	Uint64	serialNumber;
	//0x50
	Uint32	checksum;
};