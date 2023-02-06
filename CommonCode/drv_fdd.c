/*
 AcessOS 0.1
 Floppy Disk Access Code
*/
#define	MODULE_ID	FDD1
#include <kmod.h>

#define DEBUG	0
#define WARN	0

#define USE_CACHE	1
#define	CACHE_SIZE	32

#if DEBUG
# define	DEBUGS(v...)	LogF(v)
#else
# define	DEBUGS(...)
#endif

#define FDD_SEEK_TIMEOUT	10

//=== TYPEDEFS ===
typedef struct {
	int	type;
	volatile int motorState;	//2 - On, 1 - Spinup, 0 - Off
	int	track[2];
	int	timer;
	vfs_node	node;
} t_floppyDevice;

typedef struct {
	Uint64	timestamp;
	Uint16	disk;
	Uint16	sector;	// Allows 32Mb of addressable space (Plenty for FDD)
	char	data[512];
} t_floppySector;

//=== CONSTANTS ===
static const char *cFDD_TYPES[] = { "None", "360kB 5.25 In.", "1.2MB 5.25 In.", "720kB 3.5 In", "1.44MB 3.5 In.", "2.88MB 3.5 In." };
static const int cFDD_SIZES[] = { 0, 360*1024, 1200*1024, 720*1024, 1440*1024, 2880*1024 };
static const short	cPORTBASE[] = { 0x3F0, 0x370 };
#define MOTOR_ON_DELAY	500		//Miliseconds
#define MOTOR_OFF_DELAY	2000	//Miliseconds

enum FloppyPorts {
	PORT_STATUSA	= 0x0,
	PORT_STATUSB	= 0x1,
	PORT_DIGOUTPUT	= 0x2,
	PORT_MAINSTATUS	= 0x4,
	PORT_DATARATE	= 0x4,
	PORT_DATA		= 0x5,
	PORT_DIGINPUT	= 0x7,
	PORT_CONFIGCTRL	= 0x7
};

enum FloppyCommands {
	FIX_DRIVE_DATA	= 0x03,
	HECK_DRIVE_STATUS	= 0x04,
	CALIBRATE_DRIVE	= 0x07,
	CHECK_INTERRUPT_STATUS = 0x08,
	SEEK_TRACK		= 0x0F,
	READ_SECTOR_ID	= 0x4A,
	FORMAT_TRACK	= 0x4D,
	READ_TRACK		= 0x42,
	READ_SECTOR		= 0x66,
	WRITE_SECTOR	= 0xC5,
	WRITE_DELETE_SECTOR	= 0xC9,
	READ_DELETE_SECTOR	= 0xCC,
};

//=== FUNCTION PROTOTYPES ===
static int	fdd_readSector(int disk, int lba, void *buf);
static int	fdd_ioctlFS(vfs_node *node, int id, void *data);
void	FDD_WaitIRQ();
void	FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl);
static void	FDD_AquireSpinlock();
static void	inline FDD_FreeSpinlock();
#if USE_CACHE
static inline void FDD_AquireCacheSpinlock();
static inline void FDD_FreeCacheSpinlock();
#endif
static void	sendbyte(int base, char byte);
static int	getbyte(int base);
static int	seekTrack(int disk, int head, int track);
static void	stop_motor(int disk);
static void	start_motor(int disk);
static int get_dims(int type, int lba, int *c, int *h, int *s, int *spt);
static vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos);

//=== GLOBAL VARIABLES ===
const char	ModuleIdent[4] = "FDD1";
static t_floppyDevice	fdd_devices[2];
static volatile int	fdd_inUse = 0;
static volatile int	fdd_irq6 = 0;
static int	fdd_driverID = -1;
static devfs_driver	gFDD_DrvInfo = {
		{0}, fdd_ioctlFS
	};
#if USE_CACHE
static int	siFDD_CacheInUse = 0;
//static int	siFDD_SectorCacheSize = 0;
static int	siFDD_SectorCacheSize = CACHE_SIZE;
//static t_floppySector	*sFDD_SectorCache;
static t_floppySector	sFDD_SectorCache[CACHE_SIZE];
#endif

//=== CODE ===
/*
vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos)
 Read Directory Routing (for vfs_node)
 */
vfs_node *fdd_readdirFS(vfs_node *dirNode, int pos)
{
	//Update Accessed Time
	//gFDD_DrvInfo.rootNode.atime = now();
	
	//Check for bounds
	if(pos >= 2 || pos < 0)
		return NULL;
	
	if(fdd_devices[pos].type == 0)
		return &NULLNode;
	
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
	
	DEBUGS("fdd_finddirFS: Open '%s'\n", filename);
	
	if(filename == NULL)
		return NULL;
	
	//Check string length (should be 1)
	if(filename[1] != '\0')
		return NULL;
	//Get First char
	i = filename[0] - '0';
	
	//Check for 1st disk and if it is present return
	if(i == 0 && fdd_devices[0].type != 0)
		return &fdd_devices[0].node;
	
	//Check for 2nd disk and if it is present return
	if(i == 1 && fdd_devices[1].type != 0)
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
	int	disk;
	Uint32	buf[128];
	
	DEBUGS("fdd_readFS: (off=0x%x,len=0x%x,buffer=*0x%x)\n", off, len, buffer);
	
	if(node == NULL)
		return -1;
	
	if(node->inode != 0 && node->inode != 1)
		return -1;
	
	disk = node->inode;
	
	//Update Accessed Time
	node->atime = now();
	
	if((off & 0x1FF) || (len & 0x1FF))
	{
		//Un-Aligned Offset/Length
		int		startOff = off>>9;
		int		sectOff = off&0x1FF;
		int		sectors = (len+0x1FF)>>9;
	
		DEBUGS(" fdd_readFS: Non-aligned Read\n");
		
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
		
		DEBUGS("fdd_readFS: RETURN %i\n", len);
		return len;
	}
	else
	{
		int count = len >> 9;
		int	sector = off >> 9;
		DEBUGS(" fdd_readFS: Aligned Read\n");
		//Aligned Offset and Length - Simple Code
		for(i=0;i<count;i++)
		{
			fdd_readSector(disk, sector, buf);
			memcpy(buffer, buf, 512);
			buffer += 512;
			sector++;
		}
		DEBUGS("fdd_readFS: RETURN %i\n", len);
		return len;
	}
}

/*
static int fdd_readSector(int disk, int lba, void *buf)
- Read a sector from disk
*/
int fdd_readSector(int disk, int lba, void *buf)
{
	 int	cyl, head, sec;
	 int	spt, base;
	 int	i;
	
	DEBUGS("fdd_readSector: (disk=%i,lba=0x%x,buf=*0x%x)\n", disk, lba, buf);
	
	#if USE_CACHE
	FDD_AquireCacheSpinlock();
	for(i=0;i<siFDD_SectorCacheSize;i++)
	{
		if(sFDD_SectorCache[i].timestamp == 0)	continue;
		if(sFDD_SectorCache[i].disk == disk
		&& sFDD_SectorCache[i].sector == lba) {
			DEBUGS(" fdd_readSector: Found %i in cache %i\n", lba, i);
			memcpyda(buf, sFDD_SectorCache[i].data, 512/4);
			sFDD_SectorCache[i].timestamp = unow();
			FDD_FreeCacheSpinlock();
			return 1;
		}
	}
	DEBUGS(" fdd_readSector: Read %i from Disk\n", lba);
	FDD_FreeCacheSpinlock();
	#endif
	
	base = cPORTBASE[disk>>1];
	
	//Remove Old Timer
	Time_RemoveTimer(fdd_devices[disk].timer);
	//Check if Motor is on
	if(fdd_devices[disk].motorState == 0) {
		start_motor(disk);
	}
	
	DEBUGS(" fdd_readSector: Wait for Motor Spinup\n");
	
	//Wait for spinup
	while(fdd_devices[disk].motorState == 1)	Proc_Yield();
	
	DEBUGS(" fdd_readSector: Calculating Disk Dimensions\n");
	//Get CHS position
	if(get_dims(fdd_devices[disk].type, lba, &cyl, &head, &sec, &spt) != 1)
		return -1;
	
	DEBUGS(" fdd_readSector: C:%i,H:%i,S:%i\n", cyl, head, sec);
	DEBUGS(" fdd_readSector: Accquire Spinlock\n");
	
	FDD_AquireSpinlock();
		
	//Check for 2nd disk and if it is present return
	// Seek top track
	outb(base+CALIBRATE_DRIVE, 0);
	//Seek to cylinder
	i = 0;
	while(seekTrack(disk, head, (Uint8)cyl) == 0 && i++ < FDD_SEEK_TIMEOUT )	Proc_Yield();
	
	DEBUGS(" fdd_readSector: Setting DMA for read\n");
	
	//Read Data from DMA
	DMA_SetChannel(2, 512, 1);	//Read 512 Bytes
	
	DEBUGS(" fdd_readSector: Sending read command\n");
	
	Proc_Delay(100);	//Wait for Head to settle
	sendbyte(base, READ_SECTOR);	//Was 0xE6
	sendbyte(base, (head << 2) | (disk&1));
	sendbyte(base, (Uint8)cyl);
	sendbyte(base, (Uint8)head);
	sendbyte(base, (Uint8)sec);
	sendbyte(base, 0x02);	// Bytes Per Sector (Real BPS=128*2^{val})
	sendbyte(base, spt);	// SPT
	sendbyte(base, 0x1B);	// Gap Length (27 is default)
	sendbyte(base, 0xFF);	// Data Length
	
	#if DEBUG
		LogF(" fdd_readSector: Waiting for Data to be read\n");
	#endif
	// Wait for IRQ
	FDD_WaitIRQ();
	
	#if DEBUG
		LogF(" fdd_readSector: Reading Data\n");
	#endif
	//Read Data from DMA
	DMA_ReadData(2, 512, buf);
	
	#if DEBUG
		LogF(" fdd_readSector: Clearing Input Buffer\n");
	#endif
	//Clear Input Buffer
	getbyte(base); getbyte(base); getbyte(base);
	getbyte(base); getbyte(base); getbyte(base); getbyte(base);
	
	#if DEBUG
		LogF(" fdd_readSector: Realeasing Spinlock and Setting motor to stop\n");
	#endif
	// Release Spinlock
	FDD_FreeSpinlock();
	
	//Set timer to turn off motor affter a gap
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, stop_motor, disk, 1);	//One Shot Timer

	#if USE_CACHE
	{
		FDD_AquireCacheSpinlock();
		int oldest = 0;
		for(i=0;i<siFDD_SectorCacheSize;i++)
		{
			if(sFDD_SectorCache[i].timestamp == 0) {
				oldest = i;
				break;
			}
			if(sFDD_SectorCache[i].timestamp < sFDD_SectorCache[oldest].timestamp)
				oldest = i;
		}
		sFDD_SectorCache[oldest].timestamp = unow();
		sFDD_SectorCache[oldest].disk = disk;
		sFDD_SectorCache[oldest].sector = lba;
		memcpyda(sFDD_SectorCache[oldest].data, buf, 512/4);
		FDD_FreeCacheSpinlock();
	}
	#endif

	DEBUGS("fdd_readSector: RETURN 1\n");
	return 1;
}

/*
static int seekTrack(int disk, int track)
- Seek disk to selected track
*/
static int seekTrack(int disk, int head, int track)
{
	Uint8	sr0, cyl;
	 int	base;
	
	base = cPORTBASE[disk>>1];
	
	// Check if seeking is needed
	if(fdd_devices[disk].track[head] == track)
		return 1;
	
	// - Seek Head 0
	sendbyte(base, SEEK_TRACK);
	sendbyte(base, (head<<2)|(disk&1));
	sendbyte(base, track);	// Send Seek command
	FDD_WaitIRQ();
	FDD_SensInt(base, &sr0, &cyl);	// Wait for IRQ
	if((sr0 & 0xF0) != 0x20) {
		DEBUGS(" seekTrack: sr0=0x%x\n", sr0);
		return 0;	//Check Status
	}
	if(cyl != track)	return 0;
	
	// Set Track in structure
	fdd_devices[disk].track[head] = track;
	return 1;
}

/*
 Get Dimensions of a disk
*/
static int get_dims(int type, int lba, int *c, int *h, int *s, int *spt)
{
	switch(type) {
	case 0:
		return 0;
	
	// 360Kb 5.25"
	case 1:
		*spt = 9;
		*s = (lba % 9) + 1;
		*c = lba / 18;
		*h = (lba / 9) & 1;
		break;
	
	// 1220Kb 5.25"
	case 2:
		*spt = 15;
		*s = (lba % 15) + 1;
		*c = lba / 30;
		*h = (lba / 15) & 1;
		break;
	
	// 720Kb 3.5"
	case 3:
		*spt = 9;
		*s = (lba % 9) + 1;
		*c = lba / 18;
		*h = (lba / 9) & 1;
		break;
	
	// 1440Kb 3.5"
	case 4:
		*spt = 18;
		*s = (lba % 18) + 1;
		*c = lba / 36;
		*h = (lba / 18) & 1;
		break;
		
	// 2880Kb 3.5"
	case 5:
		*spt = 36;
		*s = (lba % 36) + 1;
		*c = lba / 72;
		*h = (lba / 32) & 1;
		break;
		
	default:
		return -2;
	}
	return 1;
}

/*
- Dummy ioctl function
*/
static int fdd_ioctlFS(vfs_node *node, int id, void *data)
{
	return 0;
}

/*
void fdc_handler()
- Handles IRQ6
*/
void fdd_handler(t_regs* unused)
{
    fdd_irq6 = 1;
}

void FDD_WaitIRQ()
{
	// Wait for IRQ
	while(!fdd_irq6)	Proc_Yield();
	fdd_irq6 = 0;
}
void FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl)
{
	sendbyte(base, CHECK_INTERRUPT_STATUS);
	if(sr0)	*sr0 = getbyte(base);
	else	getbyte(base);
	if(cyl)	*cyl = getbyte(base);
	else	getbyte(base);
}

void	FDD_AquireSpinlock()
{
	while(fdd_inUse)
		Proc_Yield();
	fdd_inUse = 1;
}

inline void FDD_FreeSpinlock()
{
	fdd_inUse = 0;
}

#if USE_CACHE
inline void FDD_AquireCacheSpinlock()
{
	while(siFDD_CacheInUse)	Proc_Yield();
	siFDD_CacheInUse = 1;
}
inline void FDD_FreeCacheSpinlock()
{
	siFDD_CacheInUse = 0;
}
#endif

/*
void sendbyte(int base, char byte)
- Sends a command to the controller
- From the Intel manual
*/
static void sendbyte(int base, char byte)
{
    volatile int state;
    int timeout = 128;
    for( ; timeout--; )
    {
        state = inb(base + PORT_MAINSTATUS);
        if ((state & 0xC0) == 0x80)
        {
            outb(base + PORT_DATA, byte);
            return;
        }
        inb(0x80);	//Delay
    }
	#if WARN
		warning("FDD_SendByte - Timeout sending byte 0x%x to base 0x%x\n", byte, base);
	#endif
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
        state = inb((base + PORT_MAINSTATUS));
        if ((state & 0xd0) == 0xd0)
	        return inb(base + PORT_DATA);
        inb(0x80);
    }
    return -1;
}

void FDD_Recalibrate(int disk)
{
	#if DEBUG
		LogF("FDD_Recalibrate: (disk=%i)\n", disk);
		LogF(" FDD_Recalibrate: Starting Motor\n");
	#endif
	start_motor(disk);
	// Wait for Spinup
	while(fdd_devices[disk].motorState == 1)	Proc_Yield();
	
	#if DEBUG
		LogF(" FDD_Recalibrate: Sending Calibrate Command\n");
	#endif
	sendbyte(cPORTBASE[disk>>1], CALIBRATE_DRIVE);
	sendbyte(cPORTBASE[disk>>1], disk&1);
	
	#if DEBUG
		LogF(" FDD_Recalibrate: Waiting for IRQ\n");
	#endif
	FDD_WaitIRQ();
	FDD_SensInt(cPORTBASE[disk>>1], NULL, NULL);
	
	#if DEBUG
		LogF(" FDD_Recalibrate: Stopping Motor\n");
	#endif
	stop_motor(disk);
	DEBUGS("FDD_Recalibrate: RETURN\n");
}

void FDD_Reset(int id)
{
	int base = cPORTBASE[id];
	
	#if DEBUG
		LogF("FDD_Reset: (id=%i)\n", id);
	#endif
	
	outb(base + PORT_DIGOUTPUT, 0);	// Stop Motors & Disable FDC
	outb(base + PORT_DIGOUTPUT, 0x0C);	// Re-enable FDC (DMA and Enable)
	
	#if DEBUG
		LogF(" FDD_Reset: Awaiting IRQ\n", id);
	#endif
	
	FDD_WaitIRQ();
	FDD_SensInt(base, NULL, NULL);
	
	#if DEBUG
		LogF(" FDD_Reset: Setting Driver Info\n", id);
	#endif
	outb(base + PORT_DATARATE, 0);	// Set data rate to 500K/s
	sendbyte(base, FIX_DRIVE_DATA);	// Step and Head Load Times
	sendbyte(base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	sendbyte(base, 0x02);	// Head Load Time >> 1
	while(seekTrack(0, 0, 1) == 0);	// set track
	while(seekTrack(0, 1, 1) == 0);	// set track
	
	#if DEBUG
	LogF(" FDD_Reset: Recalibrating Disk\n", id);
	#endif
	FDD_Recalibrate((id<<1)|0);
	FDD_Recalibrate((id<<1)|1);

	DEBUGS("FDD_Reset: RETURN\n");
}

/*
void fdd_timer()
- Called by timer
*/
static void fdd_timer(int arg)
{
	DEBUGS("fdd_timer: (arg=%i)\n", arg);
	if(fdd_devices[arg].motorState == 1)
		fdd_devices[arg].motorState = 2;
	Time_RemoveTimer(fdd_devices[arg].timer);
	fdd_devices[arg].timer = -1;
}

/*
void start_motor(char disk)
- Starts FDD Motor
*/
static void start_motor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state |= 1 << (4+disk);
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
	fdd_devices[disk].motorState = 1;
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_ON_DELAY, fdd_timer, disk, 1);	//One Shot Timer
}

/*
void stop_motor(int disk)
- Stops FDD Motor
*/
static void stop_motor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state &= ~( 1 << (4+disk) );
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
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
	fdd_devices[0].track[0] = -1;
	fdd_devices[1].track[1] = -1;
	
	// Clear FDD IRQ Flag
	FDD_SensInt(0x3F0, NULL, NULL);
	//Install IRQ6 Handler
	IRQ_Set(6, fdd_handler);
	// Reset Primary FSS Controller
	FDD_Reset(0);
	
	LogF("[FDD ] Detected Disk 0: %s and Disk 1: %s\n", cFDD_TYPES[data>>4], cFDD_TYPES[data&0xF]);
	
	//Initialise Root Node
	gFDD_DrvInfo.rootNode.name = "fdd";
	gFDD_DrvInfo.rootNode.nameLength = 3;
	gFDD_DrvInfo.rootNode.inode = 0;	gFDD_DrvInfo.rootNode.length = 2;
	gFDD_DrvInfo.rootNode.uid = 0;		gFDD_DrvInfo.rootNode.gid = 0;
	gFDD_DrvInfo.rootNode.mode = 0111;	gFDD_DrvInfo.rootNode.flags = VFS_FFLAG_DIRECTORY;
	gFDD_DrvInfo.rootNode.ctime = gFDD_DrvInfo.rootNode.mtime
		= gFDD_DrvInfo.rootNode.atime = now();
	gFDD_DrvInfo.rootNode.read = NULL;		gFDD_DrvInfo.rootNode.write = NULL;
	gFDD_DrvInfo.rootNode.readdir = fdd_readdirFS;
	gFDD_DrvInfo.rootNode.finddir = fdd_finddirFS;
	gFDD_DrvInfo.rootNode.mknod = NULL;	gFDD_DrvInfo.rootNode.unlink = NULL;

	//Initialise Driver structure and Register with devfs
	gFDD_DrvInfo.ioctl = fdd_ioctlFS;
	fdd_driverID = DevFS_AddDevice(&gFDD_DrvInfo);
	if(fdd_driverID == -1) {
		return 1;
	}
	gFDD_DrvInfo.rootNode.impl = fdd_driverID;
	
	//Initialise Child Nodes
	memcpy(&fdd_devices[0].node, &gFDD_DrvInfo.rootNode, sizeof(vfs_node));
	fdd_devices[0].node.inode = 0;
	fdd_devices[0].node.name = "0";	fdd_devices[0].node.nameLength = 1;
	fdd_devices[0].node.flags = 0;
	fdd_devices[0].node.read = fdd_readFS;	fdd_devices[0].node.write = NULL;//fdd_writeFS;
	fdd_devices[0].node.readdir = NULL;	fdd_devices[0].node.finddir = NULL;
	memcpy(&fdd_devices[1].node, &fdd_devices[0].node, sizeof(vfs_node));
	fdd_devices[1].node.name = "1";
	fdd_devices[1].node.inode = 1;
	
	//Set Lengths
	fdd_devices[0].node.length = cFDD_SIZES[data >> 4];
	fdd_devices[1].node.length = cFDD_SIZES[data & 0xF];
	
	// Create Sector Cache
	#if USE_CACHE
	//sFDD_SectorCache = malloc(sizeof(*sFDD_SectorCache)*CACHE_SIZE);
	//siFDD_SectorCacheSize = CACHE_SIZE;
	#endif
	
	return 0;
}

void ModuleUnload()
{
	int i;
	FDD_AquireSpinlock();
	for(i=0;i<4;i++) {
		Time_RemoveTimer(fdd_devices[i].timer);
		stop_motor(i);
	}
	IRQ_Clear(6);
}
