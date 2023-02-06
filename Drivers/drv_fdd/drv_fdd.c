/*
 AcessOS 0.1
 Floppy Disk Access Code
 Based on code from http://wiki.osdev.org/User:Omega
*/
#include <kmod.h>

#define DEBUG	1

//=== TYPEDEFS ===
typedef struct {
	int	type;
	int motorState;	//Bool: 1 - On, 0 - Off
	int	track;
	int	timer;
	vfs_node	node;
} t_floppyDevice;

//=== CONSTANTS ===
const char *cFDD_TYPES[] = { "None", "360kB 5.25 In.", "1.2MB 5.25 In.", "720kB 3.5 In", "1.44MB 3.5 In", "2.88MB 3.5 In." };
const int cFDD_SIZES[] = { 0, 360*1024, 1200*1024, 720*1024, 1440*1024, 2880*1024 };
const short	cPORTBASE[] = { 0x3F0 };
#define MOTOR_ON_DELAY	0.5f	//Seconds
#define MOTOR_OFF_DELAY	2.0f	//Seconds

//=== FUNCTION PROTOTYPES ===
static int fdd_readSector(int disk, int lba, void *buf);
static int fdd_ioctlFS(vfs_node *node, int id, void *data);
static void sendbyte(int base, char byte);
static int getbyte(int base);
static int seekTrack(int disk, int track);
static void stop_motor(int disk);
static void start_motor(int disk);
vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos);

//=== GLOBAL VARIABLES ===
const char	ModuleIdent[4] = "FDD1";
t_floppyDevice	fdd_devices[2];
vfs_node	fdd_rootNode;
volatile int	fdd_inUse = 0;
int	fdd_driverID = -1;
volatile int	fdd_irq6 = 0;
devfs_driver	fdd_drvStruct = {
	"fdd", 3, &fdd_rootNode, fdd_ioctlFS
};

//=== CODE ===
/*
vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos)
 Read Directory Routing (for vfs_node)
 */
vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos)
{
	//Update Accessed Time
	fdd_rootNode.atime = now();
	
	//Check for bounds
	if(pos >= 2 || pos < 0)
		return NULL;
	
	//Return
	return &fdd_devices[pos].node;
}

/*
vfs_node *fdd_finddirFS(vfs_node *dirNode, char *filename);
 Find File Routine (for vfs_node)
 */
vfs_node *fdd_finddirFS(vfs_node *dirNode, char *filename)
{
	int i;
	
	#if DEBUG
	//printf("fdd_finddirFS: Open '%s'\n", filename);
	#endif
	
	//Update accessed Time
	fdd_rootNode.atime = now();
	
	//Check string length (should be 1)
	if(filename[1] != '\0')
		return NULL;
	//Get First char
	i = filename[0] - '0';
	
	//Check for 1st disk and if it is present return
	if(i == 1 && fdd_devices[0].type != 0)
		return &fdd_devices[0].node;
	
	//Check for 2nd disk and if it is present return
	if(i == 2 && fdd_devices[1].type != 0)
		return &fdd_devices[1].node;
	//Else return null
	return NULL;
}

/*
int fdd_readFS(vfs_node *node, int off, int len, void *buffer)
- Read Data from a disk
*/
int fdd_readFS(vfs_node *node, int off, int len, void *buffer)
{
	int i = 0;
	int	disk = node->inode-1;
	Uint32	buf[128];
	
	if(node->inode != 1 && node->inode != 2)
		return -1;
	
	while(fdd_inUse) i++;	//i++ is not actually needed but is used to keep loop in existance
	
	//Update Accessed Time
	node->atime = now();
	
	if((off & 0x1FF) || (len & 0x1FF))
	{
		//Un-Aligned Offset/Length
		int		startOff = off>>9;
		int		sectOff = off&0x1FF;
		int		sectors = (len+0x1FF)>>9;
	
		#if DEBUG
		//printf("fdd_readFS: Non-aligned Read\n");
		#endif
		
		//Read Starting Sector
		if(!fdd_readSector(disk, startOff, buf))
			return 0;
		memcpy(buffer, (char*)(buf+sectOff), len>512-sectOff?512-sectOff:len);
		
		//If the data size is one sector or less
		if(len <= 512-sectOff)
			return 1;	//Return
		buffer += 512-sectOff;
	
		//Read Middle Sectors
		for(i=1;i<sectors-1;i++)	{
			if(!fdd_readSector(disk, startOff+i, buf))
				return 0;
			memcpy(buffer, buf, 512);
			buffer += 512;
		}
	
		//Read End Sectors
		if(!fdd_readSector(disk, startOff+i, buf))
			return 0;
		memcpy(buffer, buf, (len&0x1FF)-sectOff);
		
		return 1;
	} else {
		int count = len >> 9;
		int	sector = off >> 9;
		#if DEBUG
		//printf("fdd_readFS: Aligned Read\n");
		#endif
		//Aligned Offset and Length - Simple Code
		for(i=0;i<count;i++) {
			fdd_readSector(disk, sector, buf);
			memcpy(buffer, buf, 512);
			buffer += 512;
			sector++;
		}
		return 1;
	}
}

/*
static int fdd_readSector(int disk, int lba, void *buf)
- Read a sector from disk
*/
static int fdd_readSector(int disk, int lba, void *buf)
{
	int cyl, head, sec;
	int	i;
	
	//Check if Motor is on
	if(fdd_devices[disk].motorState == 0) {
		start_motor(disk);
	}
	
	//Wait for spinup
	i = 0;
	while(fdd_devices[disk].motorState == 1) i++;
	
	//Clear Timer
	Time_RemoveTimer(fdd_devices[disk].timer);
	fdd_devices[disk].timer = -1;
	
	//Get CHS position
	switch(fdd_devices[disk].type) {
	case 0:
		return -1;
	
	case 4:
		sec = (lba % 18) + 1;
		cyl = lba / 36;
		head = (lba / 18) % 2;
		break;
		
	default:
		return -2;
	}
	
	//Set In Use Flag
	fdd_inUse = 1;
	
	//Seek to cylinder
	outb(0x3F7, 0);
	while(seekTrack(disk, (char)cyl) == 0);
	sendbyte(0x3F0, 0xE6);
	sendbyte(0x3F0, ((unsigned char)head << 2) | (disk&0x1));
	sendbyte(0x3F0, (unsigned char)cyl);
	sendbyte(0x3F0, (unsigned char)head);
	sendbyte(0x3F0, (unsigned char)sec);
	sendbyte(0x3F0, 0x02);
	sendbyte(0x3F0, 0x12);
	sendbyte(0x3F0, 0x1B);
	sendbyte(0x3F0, 0xFF);
	
	//Wait for transfer to finish
	i = 0;
	while(fdd_inUse) i++;
	
	//Clear Input Buffer
	getbyte(0x3F0); getbyte(0x3F0); getbyte(0x3F0);
	getbyte(0x3F0); getbyte(0x3F0); getbyte(0x3F0); getbyte(0x3F0);
	
	//Read Data from DMA
	DMA_ReadData(2, 512, buf);
	
	//Set timer to turn off motor affter a gap
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, stop_motor, disk, 1);	//One Shot Timer
	return 0;
}

/*
- Dummy ioctl function
*/
static int fdd_ioctlFS(vfs_node *node, int id, void *data)
{
	return 0;
}

/*
void sendbyte(int base, char byte)
- Sends a command to the controller
- From the Intel manual
*/
static void sendbyte(int base, char byte)
{
    volatile int state;
    int timeout;
    for( timeout = 128; timeout--; )
    {
        state = inb((base + 0x04));
        if ((state & 0xC0) == 0x80)
        {
            outb((base + 0x05), byte);
            return;
        }
        inb(0x80);
    }
}

/*
int getbyte(int base, char byte)
- Receive data from fdd controller
- From the Intel manual
*/
static int getbyte(int base)
{
    volatile int state;
    int timeout;
    for( timeout = 128; timeout--; )
    {
        state = inb((base + 0x04));
        if ((state & 0xd0) == 0xd0)
	        return inb((base + 0x05));
        inb(0x80);
    }
    return -1;
}

/*
static int seekTrack(int disk, int track)
- Seek disk to selected track
*/
static int seekTrack(int disk, int track)
{
	char	sr0;
	
	// Check if seeking is needed
	if(fdd_devices[disk].track == track)
		return 0;
	
	// Send Seek command
	sendbyte(0x3F0, 0x0F);
	sendbyte(0x3F0, 0x00);
	sendbyte(0x3F0, track);
	
	// Wait for IRQ
	while(!fdd_irq6);
	// Clear IRQ Flag
	fdd_irq6 = 0;
	
	//Get status
	sendbyte(0x3F0, 0x08);
	sr0 = getbyte(0x3F0);
	getbyte(0x3F0);
	
	//Check Status
	if(sr0 != 0x20)
		return -1;
	
	// Set Track in structure
	fdd_devices[disk].track = track;
	return 0;
}

/*
void fdc_handler()
- Handles IRQ6
*/
static void fdd_handler(t_regs *r)
{
    fdd_irq6 = 1;
}

/*
void fdd_timer()
- Called by timer
*/
static void fdd_timer(int arg)
{
	if(fdd_devices[arg].motorState == 1)
		fdd_devices[arg].motorState = 2;
}

/*
void start_motor(char disk)
- Starts FDD Motor
*/
static void start_motor(int disk)
{
    outb( (cPORTBASE[ disk>>1 ] + 0x2), 0x1C );
    fdd_devices[disk].motorState = 1;
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_ON_DELAY, fdd_timer, disk, 1);	//One Shot Timer
}

/*
void start_motor(int disk)
- Stops FDD Motor
*/
static void stop_motor(int disk)
{
    outb( (cPORTBASE[ disk>>1 ] + 0x2), 0x00 );
    fdd_devices[disk].motorState = 0;
}

/*
void fdd_install()
- Installs floppy driver
*/
int ModuleLoad()
{
	Uint8 data;
	
	//Determine Floppy Types
	outb(0x70, 0x10);
	data = inb(0x71);
	fdd_devices[0].type = data >> 4;
	fdd_devices[1].type = data & 0xF;
	
	//Initialise Root Node
	fdd_rootNode.name = fdd_drvStruct.name;
	fdd_rootNode.nameLength = fdd_drvStruct.nameLen;
	fdd_rootNode.inode = 0;		fdd_rootNode.length = 2;
	fdd_rootNode.uid = 0;		fdd_rootNode.gid = 0;
	fdd_rootNode.mode = 0775;	fdd_rootNode.flags = VFS_FFLAG_DIRECTORY;
	fdd_rootNode.ctime = fdd_rootNode.mtime = fdd_rootNode.atime = now();
	fdd_rootNode.read = NULL;	fdd_rootNode.write = NULL;
	fdd_rootNode.readdir = fdd_readdirFS;
	fdd_rootNode.finddir = fdd_finddirFS;
	fdd_rootNode.mknod = NULL;	fdd_rootNode.unlink = NULL;

	//Initialise Driver structure and Register with devfs
	fdd_drvStruct.rootNode = &fdd_rootNode;
	fdd_drvStruct.ioctl = fdd_ioctlFS;
	fdd_driverID = DevFS_AddDevice(&fdd_drvStruct);
	if(fdd_driverID == -1) {
		return 1;
	}
	fdd_rootNode.impl = fdd_driverID;
	
	//Initialise Child Nodes
	memcpy(&fdd_devices[0].node, &fdd_rootNode, sizeof(vfs_node));
	fdd_devices[0].node.name = "0";	fdd_devices[0].node.nameLength = 1;
	fdd_devices[0].node.flags = 0;
	fdd_devices[0].node.read = fdd_readFS;	fdd_devices[0].node.write = NULL;//fdd_writeFS;
	fdd_devices[0].node.readdir = NULL;	fdd_devices[0].node.finddir = NULL;
	memcpy(&fdd_devices[1].node, &fdd_devices[0].node, sizeof(vfs_node));
	fdd_devices[1].node.name = "1";
	
	//Set Lengths
	fdd_devices[0].node.length = cFDD_SIZES[data >> 4];
	fdd_devices[1].node.length = cFDD_SIZES[data & 0xF];
	
	//Install IRQ6 Handler
	IRQ_Set(6, fdd_handler);
	
	return 0;
}

void ModuleUnload()
{
	int i;
	while(fdd_inUse);
	for(i=0;i<4;i++) {
		Time_RemoveTimer(fdd_devices[i].timer);
		stop_motor(i);
	}
	IRQ_Clear(6);
}
