/*
AcessOS 0.1 Basic
BGA (Bochs Graphic Adapter) Driver
*/
#include <kmod.h>

#define BGA_DEBUG	0
#define DEBUG	(BGA_DEBUG | ACESS_DEBUG)

//MODULE DEFINITIONS
const char	ModuleIdent[4] = "FDD1";
#define BGA_Install	ModuleLoad
#define BGA_Uninstall	ModuleUnload

//#define INT	static
#define INT

//TYPEDEFS
typedef struct {
	Uint16	width;
	Uint16	height;
	Uint16	bpp;
	Uint16	flags;
	Uint32	fbSize;
} t_bga_mode;

//CONSTANTS
const t_bga_mode	BGA_MODES[] = {
	{640,480,8, 0, 640*480},
	{640,480,32, 0, 640*480*4},
	{800,600,8, 0, 800*600},
	{800,600,32, 0, 800*600*4},
};
#define	BGA_MODE_COUNT	(sizeof(BGA_MODES)/sizeof(BGA_MODES[0]))
#define	BGA_LFB_MAXSIZE	(1024*768*4)
#define	VBE_DISPI_BANK_ADDRESS	0xA0000
#define VBE_DISPI_LFB_PHYSICAL_ADDRESS	0xE0000000
#define VBE_DISPI_IOPORT_INDEX	0x01CE
#define	VBE_DISPI_IOPORT_DATA	0x01CF
#define	VBE_DISPI_DISABLED	0x00
#define VBE_DISPI_ENABLED	0x01
#define	VBE_DISPI_LFB_ENABLED	0x40
#define	VBE_DISPI_NOCLEARMEM	0x80
enum {
	VBE_DISPI_INDEX_ID,
	VBE_DISPI_INDEX_XRES,
	VBE_DISPI_INDEX_YRES,
	VBE_DISPI_INDEX_BPP,
	VBE_DISPI_INDEX_ENABLE,
	VBE_DISPI_INDEX_BANK,
	VBE_DISPI_INDEX_VIRT_WIDTH,
	VBE_DISPI_INDEX_VIRT_HEIGHT,
	VBE_DISPI_INDEX_X_OFFSET,
	VBE_DISPI_INDEX_Y_OFFSET
};
enum {
	BGA_IOCTL_NULL,
	BGA_IOCTL_GETMODE,
	BGA_IOCTL_SETMODE
};

//GLOBALS
vfs_node	gBgaRootNode;
devfs_driver	gBgaDriverStruct = {
		"bochsvbe", 8, 0, 0
	};
int	giBgaCurrentMode = -1;
int giBgaDriverId = -1;
Uint	*gBgaFramebuffer = (Uint*)VBE_DISPI_LFB_PHYSICAL_ADDRESS;

//PROTOTYPES
// Internal
INT void BGA_int_WriteRegister(Uint16 reg, Uint16 value);
INT Uint16 BGA_int_ReadRegister(Uint16 reg);
INT void BGA_int_SetBank(Uint16 bank);
INT void BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp);
INT void BGA_int_UpdateMode(int id);
// Filesystem
INT int	BGA_Read(vfs_node *node, int off, int len, void *buffer);
INT int	BGA_Write(vfs_node *node, int off, int len, void *buffer);
INT int	BGA_Ioctl(vfs_node *node, int id, void *data);

//CODE
int BGA_Install()
{
	int	bga_version = 0;
	
	
	LogF("[BGA ] Installing... ");
	// Check BGA Version
	bga_version = BGA_int_ReadRegister(VBE_DISPI_INDEX_ID);
	if(bga_version != 0xB0C4) {
		return 0;
	}
	
	// Create Root Node
	gBgaRootNode.name = gBgaDriverStruct.name;
	gBgaRootNode.nameLength = gBgaDriverStruct.nameLen;
	gBgaRootNode.uid = 0;	gBgaRootNode.gid = 0;
	gBgaRootNode.mode = 0664;	gBgaRootNode.flags = 0;
	gBgaRootNode.ctime = 
		gBgaRootNode.mtime = 
		gBgaRootNode.atime = now();
	gBgaRootNode.readdir = NULL;	gBgaRootNode.finddir = NULL;
	gBgaRootNode.read = BGA_Read;	gBgaRootNode.write = BGA_Write;
	gBgaRootNode.close = NULL;
	
	// Create Driver Structure
	gBgaDriverStruct.rootNode = &gBgaRootNode;
	gBgaDriverStruct.ioctl = BGA_Ioctl;
	
	// Install Device
	giBgaDriverId = DevFS_AddDevice( &gBgaDriverStruct );
	if(giBgaDriverId == -1)	return 0;
	gBgaRootNode.impl = giBgaDriverId;	//Used by DevFS
	
	// Map Framebuffer to hardware address
	gBgaFramebuffer = MM_MapHW(VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768);	//768 pages (3Mb)
	
	LogF("Done.\n");
	
	return 1;
}

/**
 \fn void BGA_Uninstall()
*/
void BGA_Uninstall()
{
	DevFS_DelDevice( giBgaDriverId );
	MM_UnmapHW( VBE_DISPI_LFB_PHYSICAL_ADDRESS, 768 );
}

/* Read from the framebuffer
 */
INT int BGA_Read(vfs_node *node, int off, int len, void *buffer)
{
	// Check Mode
	if(giBgaCurrentMode == -1)
		return -1;
	// Check Offset and Length against Framebuffer Size
	if(off+len > BGA_MODES[giBgaCurrentMode].fbSize)
		return -1;
	// Copy from Framebuffer
	if(len&3)	memcpy(buffer, (Uint32*)((Uint)gBgaFramebuffer + off), len);
	else		memcpyd(buffer, (Uint32*)((Uint)gBgaFramebuffer + off), len/4);
	return len;
}

/* Write to the framebuffer
 */
INT int BGA_Write(vfs_node *node, int off, int len, void *buffer)
{
	#if DEBUG
		LogF("BGA_Write: (off=%i, len=0x%x)\n", off, len);
	#endif
	// Check Mode
	if(giBgaCurrentMode == -1)
		return -1;
	// Check Input against Frambuffer Size
	if(off+len > BGA_MODES[giBgaCurrentMode].fbSize)
		return -1;
	
	#if DEBUG
		LogF(" BGA_Write: *buffer = 0x%x\n", *(Uint*)buffer);
		LogF(" BGA_Write: Updating Framebuffer (0x%x - 0x%x bytes)\n", 
			(Uint)gBgaFramebuffer + off, (Uint)gBgaFramebuffer + off + len);
	#endif
	
	// Copy to Frambuffer
	if(len&3 || off&3 || (Uint)buffer&3)	// Non-Aligned Data
		memcpy((Uint32*)((Uint)gBgaFramebuffer + off), buffer, len);
	else
		memcpyda((Uint32*)((Uint)gBgaFramebuffer + off), buffer, len>>2);
	#if DEBUG
		LogF(" BGA_Write: BGA Framebuffer updated\n");
	#endif
	
	return len;
}

/* Handle messages to the device
 */
INT int BGA_Ioctl(vfs_node *node, int id, void *data)
{
	int	intData = *(int*)(data);
	
	#if DEBUG
		LogF("BGA_Ioctl: (id=%i, data=0x%x)\n", id, intData);
	#endif
	
	switch(id) {
	
	case BGA_IOCTL_NULL:
		return -2;
		
	case BGA_IOCTL_GETMODE:
		if(intData < BGA_MODE_COUNT) {
			return (Uint32)&BGA_MODES[intData];
		}
		return -1;
		break;
		
	case BGA_IOCTL_SETMODE:
		if(intData < BGA_MODE_COUNT) {
			BGA_int_UpdateMode(intData);
			return intData;
		}
		return -1;
		break;
	
	default:
		return -2;
		break;
	
	}
}

//== Internal Functions ==
INT void BGA_int_WriteRegister(Uint16 reg, Uint16 value)
{
	outw(VBE_DISPI_IOPORT_INDEX, reg);
	outw(VBE_DISPI_IOPORT_DATA, value);
}

INT Uint16 BGA_int_ReadRegister(Uint16 reg)
{
	outw(VBE_DISPI_IOPORT_INDEX, reg);
	return inw(VBE_DISPI_IOPORT_DATA);
}

#if 0
INT void BGA_int_SetBank(Uint16 bank)
{
	BGA_int_WriteRegister(VBE_DISPI_INDEX_BANK, bank);
}
#endif

INT void BGA_int_SetMode(Uint16 width, Uint16 height, Uint16 bpp)
{
	BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_XRES,	width);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_YRES,	height);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_BPP,	bpp);
    BGA_int_WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

INT void BGA_int_UpdateMode(int id)
{
	BGA_int_SetMode(BGA_MODES[id].width, BGA_MODES[id].height, BGA_MODES[id].bpp);
	giBgaCurrentMode = id;
}
