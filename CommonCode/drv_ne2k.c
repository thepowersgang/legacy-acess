/*
AcessOS Version 1
- Ne2k Driver
*/
#define	MODULE_ID	Ne2k
#include <kmod.h>

#define Ne2k_DEBUG	0
#define DEBUG	(ACESS_DEBUG|NE2K_DEBUG)

// === CONSTANTS ===
static const struct {
	Uint16	Vendor;
	Uint16	Device;
} csaCOMPAT_DEVICES[] = {
	{0x10EC, 0x8029},
	{0x10EC, 0x8129}
};
#define NUM_COMPAT_DEVICES	(sizeof(csaCOMPAT_DEVICES)/sizeof(csaCOMPAT_DEVICES[0]))

enum ePortsRead {
	CMD = 0x00,
	CLDA0,	// Current Local DMA Address
	CLDA1,
	BNRY,	// Boundry Address
	TSR,	// Transmit Status Register
	NCMD,	// Collisions Counter
	FIFO,	// ???
	ISR,	// Interrupt Status Register
	CRDA0,	// Current Remote DMA Address
	CRDA1,
	RSR = 0x0C,	// Recieve Status Register
	
	CR_1 = 0x20,
	MAC0, MAC1, MAC2,
	MAC3, MAC4, MAC5,
	CURR,
	MAR0, MAR1, MAR2, MAR3,
	MAR4, MAR5, MAR6, MAR7,
	
	LAST_REGISTER
};
enum ePortsWrite {
	PSTART = 0x01,	// Page Start
	PSTOP,	// Page Stop
	TPSR = 0x04,	// Transmit Page Start Address
	TBCR0,	// Transmit Byte Count
	TBCR1,
	RSAR0 = 0x8,	// Remote Start Address
	RSAR1,
	RBCR0,	// Remote Byte Count
	RBCR1,
	RCR,	// Recieve Config Register
	TCR,	// Transmit Config Register
	DCR,	// Data Config Register
	IMR,	// Inerrupt Mask Register
	LAST_REG
};

// === STRUCTURES ===
typedef struct {
	Uint8	IRQ;
	Uint8	CurPage;
	Uint16	PortBase;
	char	*Buffer;
	Uint8	MacAddr[6];
	vfs_node	Node;
} tCard;

// === PROTOTYPES ===
vfs_node	*Ne2k_ReadDir(vfs_node *node, int dirPos);
vfs_node	*Ne2k_FindDir(vfs_node *node, char *name);
 int	Ne2k_Write(vfs_node *node, int ofs, int len, void *buffer);
 int	Ne2k_Read(vfs_node *node, int ofs, int len, void *buffer);
 int	Ne2k_IOCtl(vfs_node *node, int id, void *data);
void	Ne2k_IRQHandler(t_regs *regs);
Uint8	Ne2k_ReadReg(Uint16 base, int reg);
void	Ne2k_WriteReg(Uint16 base, int reg, Uint8 data);

// === GLOBAL VARIABLES ===
tCard	*gpNe2k_Cards = NULL;
 int	giNe2k_CardCount = 0;
devfs_driver	gNe2k_DriverInfo = {{0}, Ne2k_IOCtl};

// === CODE ===
int Ne2k_Install()
{
	int	count, i, j, id, k;
	Uint16	base;
	char	*NameBuf;
	
	LogF("Ne2k_Install: ()\n");
	
	// --- Count Devices
	giNe2k_CardCount = 0;
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		// - Count Devices
		giNe2k_CardCount += PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
	}
	
	LogF(" Ne2k_Install: %i devices discovered.\n", giNe2k_CardCount);
	
	if(giNe2k_CardCount > 10)
	{
		warning("Ne2k_Install - %i cards disocovered, truncating to 10\n", giNe2k_CardCount);
	}
	
	NameBuf = (char*)malloc( giNe2k_CardCount * 2 );
	
	// --- Enumerate Devices
	k = 0;
	gpNe2k_Cards = malloc( giNe2k_CardCount * sizeof(tCard) );
	memsetda(gpNe2k_Cards, 0, giNe2k_CardCount * sizeof(tCard) / 4);
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		count = PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
		for( j = 0; j < count; j ++,k ++ )
		{
			id = PCI_GetDevice( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0, j );
			// Create Structure
			base = PCI_AssignPort( id, 0, 0x20 );
			gpNe2k_Cards[ k ].PortBase = base;
			gpNe2k_Cards[ k ].IRQ = PCI_GetIRQ( id );
			gpNe2k_Cards[ k ].CurPage = 0x40;	// Current Page
			gpNe2k_Cards[ k ].Buffer = malloc( 8*1024 );	// (8kb+16bytes)
			LogF(" Ne2k_Install: Card #%i: IRQ=%i, Base=0x%x\n",
				k, gpNe2k_Cards[ k ].IRQ, gpNe2k_Cards[ k ].PortBase,
				gpNe2k_Cards[ k ].Buffer);
			
			//Install IRQ6 Handler
			IRQ_Set(gpNe2k_Cards[ k ].IRQ, Ne2k_IRQHandler);
			
			// Reset Card
			outportb( base+0x1F, inportb(base+0x1F));
			while( (inportb( base+ISR ) & 0x80) == 0);
			outportb( base+ISR, 0x80 );
			// Initialise Card
			outportb( base+CMD, 0x21 );	// No DMA and Stop
			outportb( base+DCR, 0x49 );	// Set WORD mode
			outportb( base+IMR, 0x00 );
			outportb( base+ISR, 0xFF );
			outportb( base+RCR, 0x20 );	// Reciever to Monitor
			outportb( base+TCR, 0x02 );	// Transmitter OFF
			outportb( base+RBCR0, 6*4 );
			outportb( base+RBCR1, 0 );
			outportb( base+RSAR0, 0 );
			outportb( base+RSAR1, 0 );
			outportb( base+CMD, 0x0A );	// Remote Read, Start
			gpNe2k_Cards[ k ].MacAddr[0] = inportb(base+0x10);	inportb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[1] = inportb(base+0x10);	inportb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[2] = inportb(base+0x10);	inportb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[3] = inportb(base+0x10);	inportb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[4] = inportb(base+0x10);	inportb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[5] = inportb(base+0x10);	inportb(base+0x10);
			
			outportb( base+PSTART, 0x60);	// Set Receive Start
			outportb( base+BNRY, 0x7F);	// Set Boundary Page
			outportb( base+PSTOP, 0x80);	// Set Stop Page
			outportb( base+ISR, 0xFF );	// Clear all ints
			outportb( base+CMD, 0x22 );	// No DMA, Start
			outportb( base+IMR, 0x3f );	// Set Interupt Mask
			outportb( base+RCR, 0x8f );	// Set WRAP and allow all packet matches
			outportb( base+TCR, 0x00 );	// Set Normal Transmitter mode
			outportb( base+TPSR, 0x40);	// Set Transmit Start
			
			// Set MAC Address
			Ne2k_WriteReg(base, MAC0, gpNe2k_Cards[ k ].MacAddr[0]);
			Ne2k_WriteReg(base, MAC1, gpNe2k_Cards[ k ].MacAddr[1]);
			Ne2k_WriteReg(base, MAC2, gpNe2k_Cards[ k ].MacAddr[2]);
			Ne2k_WriteReg(base, MAC3, gpNe2k_Cards[ k ].MacAddr[3]);
			Ne2k_WriteReg(base, MAC4, gpNe2k_Cards[ k ].MacAddr[4]);
			Ne2k_WriteReg(base, MAC5, gpNe2k_Cards[ k ].MacAddr[5]);
			
			LogF(" Ne2k_Install: MAC Address %x:%x:%x:%x:%x:%x\n",
				gpNe2k_Cards[ k ].MacAddr[0], gpNe2k_Cards[ k ].MacAddr[1],
				gpNe2k_Cards[ k ].MacAddr[2], gpNe2k_Cards[ k ].MacAddr[3],
				gpNe2k_Cards[ k ].MacAddr[4], gpNe2k_Cards[ k ].MacAddr[5]
				);
			
			// Set VFS Node
			gpNe2k_Cards[ k ].Node.name = NameBuf+k*2;
			gpNe2k_Cards[ k ].Node.name[0] = '0'+k;
			gpNe2k_Cards[ k ].Node.name[1] = '\0';
			gpNe2k_Cards[ k ].Node.nameLength = 1;
			gpNe2k_Cards[ k ].Node.mode = 0666;
			gpNe2k_Cards[ k ].Node.ctime = now();
			gpNe2k_Cards[ k ].Node.write = Ne2k_Write;
		}
	}
	
	// --- Set Up VFS Nodes
	memsetda(&gNe2k_DriverInfo.rootNode, 0, sizeof(vfs_node)/4);
	gNe2k_DriverInfo.rootNode.name = "ne2000";
	gNe2k_DriverInfo.rootNode.nameLength = 6;
	gNe2k_DriverInfo.rootNode.flags = VFS_FFLAG_DIRECTORY;
	gNe2k_DriverInfo.rootNode.mode = 0111;	// Directory (--X--X--X)
	gNe2k_DriverInfo.rootNode.ctime = now();
	//gNe2k_DriverInfo.rootNode.readdir = Ne2k_ReadDir;
	gNe2k_DriverInfo.rootNode.finddir = Ne2k_FindDir;
	
	// --- Install Driver
	gNe2k_DriverInfo.rootNode.impl = DevFS_AddDevice( &gNe2k_DriverInfo );
	return 1;
}

/**
 \fn vfs_node *Ne2k_ReadDir(vfs_node *node, int dirPos)
 \brief Find a directory item
*/
vfs_node *Ne2k_ReadDir(vfs_node *node, int dirPos)
{
	if(dirPos < 0)	return NULL;
	if(dirPos >= giNe2k_CardCount)	return NULL;

	return &gpNe2k_Cards[dirPos].Node;
}

/**
 \fn vfs_node *Ne2k_FindDir(vfs_node *node, char *name)
 \brief Find a directory item
*/
vfs_node *Ne2k_FindDir(vfs_node *node, char *name)
{
	int id;
	if(name[0] == '\0')	return NULL;
	if(name[1] != '\0')	return NULL;
	
	if(name[0] < '0')	return NULL;
	if(name[0] > '9')	return NULL;
	
	id = name[0] - '0';
	if(id >= giNe2k_CardCount)	return NULL;
	
	return &gpNe2k_Cards[id].Node;
}

int Ne2k_Write(vfs_node *node, int ofs, int len, void *buffer)
{
	Uint16	base;
	Uint8	page;
	//Uint8	isr;
	
	LogF("Ne2k_Write: (len=%i)\n", len);
	if(len <= 0)	return 0;
	if(len >= 0x2000)	return 0;
	if(len & 1)	len ++;
	
	base = gpNe2k_Cards[ node->inode ].PortBase;
	page = gpNe2k_Cards[ node->inode ].CurPage;
	LogF(" Ne2k_Write: base=0x%x, page=0x%x\n", base, page);
	// Increment Page ID
	gpNe2k_Cards[ node->inode ].CurPage += len >> 8;
	
	#if 1
	outportb( base+RSAR0, 0x42 );	// Start Low
	outportb( base+RSAR1, 0x00 );	// Start Hi
	outportb( base+RBCR0, 0x42 );	// Count Loaw
	outportb( base+RBCR1, 0x00 );	// Count Hi
	outportb( base+CMD, 0x0A );	// Start and Read
	#endif
	
	// Start Sending
	outportb(base+CMD, 0x22);	// Start, No DMA
	outportb(base+ISR, 0x40);	// ACK Interrupt (To be sure)
	outportb(base+RBCR0, len&0xFF);	// Low Length
	outportb(base+RBCR1, len>>8);	// High Length
	outportb(base+RSAR0, 0);	// Low Start
	outportb(base+RSAR1, page);	// High Start (On Chip Page)
	outportb(base+CMD, 0x12);	// Start, Remote DMA
	__asm__ __volatile__ ("rep outsw" : : "d" (base+0x10), "S" (buffer), "c" (len>>1) );
	while( (inportb(base+ISR) & 0x40) == 0)	Proc_Yield();
	outportb(base+ISR, 0x40);	// ACK Interrupt
	
	return len;
}

int Ne2k_Read(vfs_node *node, int ofs, int len, void *buffer)
{
	return 0;
}

int Ne2k_IOCtl(vfs_node *node, int id, void *data)
{
	return 0;
}

void Ne2k_IRQHandler(t_regs *regs)
{
	LogF("Ne2k_IRQHandler: (regs={int_no:%i}\n", regs->int_no);
	return;
}

Uint8 Ne2k_ReadReg(Uint16 base, int reg)
{
	Uint8 CMD;
	CMD = inportb(base);
	if( (CMD >> 6) != ((reg>>5) & 3) )
	{
		CMD &= ~0xC0;
		CMD |= ( (reg>>5) & 3 ) << 6;
		outportb(base, CMD);
	}
	return inportb( base + (reg&0x1F) );
}
void Ne2k_WriteReg(Uint16 base, int reg, Uint8 data)
{
	Uint8 CMD;
	CMD = inportb(base);
	if( (CMD >> 6) != ((reg>>5) & 3) )
	{
		CMD &= ~0xC0;
		CMD |= ( (reg>>5) & 3 ) << 6;
		outportb(base, CMD);
	}
	outportb( base + (reg&0x1F), data );
}
