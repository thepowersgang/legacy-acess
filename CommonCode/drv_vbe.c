/*
AcessOS 1
Video BIOS Extensions (Vesa) Driver
*/
#include <acess.h>
#include <vfs.h>
#include <tpl_drv_video.h>
#include <fs_devfs2.h>

#define VESA_DEBUG	0
#define DEBUG	(VESA_DEBUG | ACESS_DEBUG)

#define VESA_VERSION_MAJOR	1
#define VESA_VERSION_MINOR	0
#define VESA_VERSION	(((VESA_VERSION_MAJOR)<<16)|((VESA_VERSION_MINOR)&0xFFFF))

//#define INT	static
#define INT

//TYPEDEFS
typedef struct {
	Uint16	off;
	Uint16	seg;
} t_farPtr;

typedef struct {
	Uint16	code;
	Uint16	width, height;
	Uint16	pitch, bpp;
	Uint16	flags;
	Uint32	fbSize;
	Uint	framebuffer;
} t_vesa_mode;

typedef struct {
	Uint16	attributes;
	Uint8	winA,winB;
	Uint16	granularity;
	Uint16	winsize;
	Uint16	segmentA, segmentB;
	t_farPtr	realFctPtr;
	Uint16	pitch; // bytes per scanline

	Uint16	Xres, Yres;
	Uint8	Wchar, Ychar, planes, bpp, banks;
	Uint8	memory_model, bank_size, image_pages;
	Uint8	reserved0;

	Uint8	red_mask, red_position;
	Uint8	green_mask, green_position;
	Uint8	blue_mask, blue_position;
	Uint8	rsv_mask, rsv_position;
	Uint8	directcolor_attributes;

	Uint32	physbase;  // your LFB address ;)
	Uint32	reserved1;
	Sint16	reserved2;
} t_vesa_modeInfo;

typedef struct VesaControllerInfo {
   char		signature[4];		// == "VESA"
   Uint16	version;			// == 0x0300 for Vesa 3.0
   t_farPtr	oemString;			// isa vbeFarPtr
   Uint8	capabilities[4];
   t_farPtr	videomodes;			// isa vbeParPtr
   Uint16	totalMemory;		// as # of 64KB blocks
} t_vesa_info;

//CONSTANTS
enum {
	VESA_IOCTL_NULL,
	VESA_IOCTL_GETMODE,
	VESA_IOCTL_SETMODE
};
#define	FLAG_LFB	0x1

//GLOBALS
devfs_driver	gVesaDriverStruct = {
		{0}, 0
	};
int	giVesaCurrentMode = -1;
int giVesaDriverId = -1;
char	*gVesaFramebuffer = (void*)0xC00A0000;
t_vesa_mode	*gVesa_Modes;
int	giVesaModeCount = 0;
int	giVesaPageCount = 0;

//PROTOTYPES
// Filesystem
INT int	Vesa_Read(vfs_node *node, int off, int len, void *buffer);
INT int	Vesa_Write(vfs_node *node, int off, int len, void *buffer);
INT int	Vesa_Ioctl(vfs_node *node, int id, void *data);
INT int Vesa_Int_SetMode(int mode);
INT int Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
INT int Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);

//CODE
int Vesa_Install()
{
	t_vesa_info	*info;
	t_vesa_modeInfo	*modeinfo;
	tRegs16	regs;
	Uint16	*modes;
	int	i;
	
	// Lock VM8086 Monitor
	VM8086_Lock();
	// Allocate Info Block
	info = VM8086_Allocate(512);
	modeinfo = VM8086_Allocate(512);
	// Set Requested Version
	memcpy(info->signature, "VBE2", 4);
	// Set Registers
	regs.ax = 0x4F00;
	regs.es = SEG((Uint)info);
	regs.di = OFS((Uint)info);
	// Call Interrupt
	VM8086_Int(0x10, &regs);
	if(regs.ax != 0x004F) {
		warning("Vesa_Install - VESA/VBE Unsupported (AX = 0x%x)\n", regs.ax);
		return -1;
	}
	
	modes = (Uint16*)VM8086_Linear(info->videomodes.seg, info->videomodes.off);
	
	// Read Modes
	for( giVesaModeCount = 1; modes[giVesaModeCount] != 0xFFFF; giVesaModeCount++ );
	gVesa_Modes = (t_vesa_mode *)malloc( giVesaModeCount * sizeof(t_vesa_mode) );
	
	// Insert Text Mode
	gVesa_Modes[0].width = 80;
	gVesa_Modes[0].height = 25;
	gVesa_Modes[0].bpp = 4;
	gVesa_Modes[0].code = 0x3;
	
	for( i = 1; i < giVesaModeCount; i++ )
	{
		// Get Mode info
		regs.ax = 0x4F01;
		regs.cx = gVesa_Modes[i].code;
		regs.es = SEG((Uint)modeinfo);
		regs.di = OFS((Uint)modeinfo);
		VM8086_Int(0x10, &regs);
		
		// Parse Info
		gVesa_Modes[i].flags = 0;
		if ( (modeinfo->attributes & 0x90) == 0x90 )
		{
			gVesa_Modes[i].flags |= FLAG_LFB;
			gVesa_Modes[i].framebuffer = modeinfo->physbase;
			gVesa_Modes[i].fbSize = modeinfo->Xres*modeinfo->Yres*modeinfo->bpp/8;
		} else {
			gVesa_Modes[i].framebuffer = 0;
			gVesa_Modes[i].fbSize = 0;
		}
		
		gVesa_Modes[i].width = modeinfo->Xres;
		gVesa_Modes[i].height = modeinfo->Yres;
		gVesa_Modes[i].bpp = modeinfo->bpp;
		
		#if DEBUG
		LogF(" Vesa_Install: 0x%x - %ix%ix%i\n",
			gVesa_Modes[i].code, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
		#endif
	}	
	// Unlock Monitor
	VM8086_Unlock();
	
	// Create Root Node
	gVesaDriverStruct.rootNode.name = "vesa";
	gVesaDriverStruct.rootNode.nameLength = 4;
	gVesaDriverStruct.rootNode.uid = 0;	gVesaDriverStruct.rootNode.gid = 0;
	gVesaDriverStruct.rootNode.mode = 0664;	gVesaDriverStruct.rootNode.flags = 0;
	gVesaDriverStruct.rootNode.ctime = 
		gVesaDriverStruct.rootNode.mtime = 
		gVesaDriverStruct.rootNode.atime = now();
	gVesaDriverStruct.rootNode.readdir = NULL;	gVesaDriverStruct.rootNode.finddir = NULL;
	gVesaDriverStruct.rootNode.read = Vesa_Read;	gVesaDriverStruct.rootNode.write = Vesa_Write;
	gVesaDriverStruct.rootNode.close = NULL;
	
	// Create Driver Structure
	gVesaDriverStruct.ioctl = Vesa_Ioctl;
	
	// Install Device
	giVesaDriverId = dev_addDevice( &gVesaDriverStruct );
	if(giVesaDriverId == -1)	return 0;
	gVesaDriverStruct.rootNode.impl = giVesaDriverId;	//Used by DevFS
	
	return 1;
}

/* Read from the framebuffer
 */
INT int Vesa_Read(vfs_node *node, int off, int len, void *buffer)
{
	#if DEBUG >= 2
	LogF("Vesa_Read: () - NULL\n");
	#endif
	return 0;
}

/* Write to the framebuffer
 */
INT int Vesa_Write(vfs_node *node, int off, int len, void *buffer)
{
	#if DEBUG >= 2
	LogF("Vesa_Write: (node=0x%x, off=0x%x, len=0x%x, buffer=0x%x)\n", node, off, len, buffer);
	#endif

	if(buffer == NULL)
		return 0;

	if( gVesa_Modes[giVesaCurrentMode].framebuffer == 0 ) {
		warning("Vesa_Write - Non-LFB Modes not yet supported.\n");
		return 0;
	}
	if(gVesa_Modes[giVesaCurrentMode].fbSize < off+len)
	{
		warning("Vesa_Write - Framebuffer Overflow\n");
		return 0;
	}
	
	if( (off&3) == 0 && (len&3) == 0)
		memcpyda(gVesaFramebuffer+off, buffer, len/4);
	else
		memcpy(gVesaFramebuffer+off, buffer, len);
	
	return len;
}

/* Handle messages to the device
 */
INT int Vesa_Ioctl(vfs_node *node, int id, void *data)
{
	int	intData = *(int*)(data);
	#if DEBUG
	LogF("Vesa_Ioctl: (id=%i, data=0x%x)\n", id, intData);
	#endif
	switch(id)
	{
	case VIDEO_IOCTL_NULL:	return 0;
	
	case VIDEO_IOCTL_SETMODE:	return Vesa_Int_SetMode(intData);
	case VIDEO_IOCTL_GETMODE:	*(int*)(data) = giVesaCurrentMode;		return 1;
	case VIDEO_IOCTL_FINDMODE:	return Vesa_Int_FindMode((tVideo_IOCtl_Mode*)data);
	case VIDEO_IOCTL_MODEINFO:	return Vesa_Int_ModeInfo((tVideo_IOCtl_Mode*)data);
	case VIDEO_IOCTL_IDENT:	memcpy("VESA", data, 4);	return 1;
	case VIDEO_IOCTL_VERSION:	return VESA_VERSION;
	}
	return 0;
}

INT int Vesa_Int_SetMode(int mode)
{
	tRegs16	regs = {0};
	
	#if DEBUG
	LogF("Vesa_Int_SetMode: (mode=%i)\n", mode);
	#endif
	
	// Sanity Check values
	if(mode < 0 || mode > giVesaModeCount)	return -1;
	
	// Check for fast return
	if(mode == giVesaCurrentMode)	return 1;
	
	regs.ax = 0x4F02;
	regs.bx = gVesa_Modes[mode].code;
	if(gVesa_Modes[mode].flags & FLAG_LFB)
		regs.bx |= 0x4000;	// Bit 14 - Use LFB
	
	//Set Mode
	VM8086_Lock();
	VM8086_Int(0x10, &regs);
	VM8086_Unlock();
	
	// Map Framebuffer
	MM_UnmapHW(gVesaFramebuffer, giVesaPageCount);
	giVesaPageCount = (gVesa_Modes[mode].fbSize + 0xFFF) >> 12;
	gVesaFramebuffer = MM_MapHW(gVesa_Modes[mode].framebuffer, giVesaPageCount);
	
	// Record Mode Set
	giVesaCurrentMode = mode;
	
	return 1;
}

INT int Vesa_Int_FindMode(tVideo_IOCtl_Mode *data)
{
	 int	i;
	 int	best = -1, bestFactor = 1000;
	 int	factor, tmp;
	#if DEBUG
	LogF("Vesa_Int_FindMode: (data={width:%i,height:%i,bpp:%i})\n", data->width, data->height, data->bpp);
	#endif
	for(i=0;i<giVesaModeCount;i++)
	{
		#if DEBUG >= 2
		LogF("Mode %i (%ix%ix%i), ", i, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
		#endif
	
		if(gVesa_Modes[i].width == data->width
		&& gVesa_Modes[i].height == data->height
		&& gVesa_Modes[i].bpp == data->bpp)
		{
			#if DEBUG >= 2
			LogF("Perfect!\n");
			#endif
			best = i;
			break;
		}
		
		tmp = gVesa_Modes[i].width * gVesa_Modes[i].height * gVesa_Modes[i].bpp;
		tmp -= data->width * data->height * data->bpp;
		tmp = tmp < 0 ? -tmp : tmp;
		factor = tmp * 100 / (data->width * data->height * data->bpp);
		
		#if DEBUG >= 2
		LogF("factor = %i\n", factor);
		#endif
		
		if(factor < bestFactor)
		{
			bestFactor = factor;
			best = i;
		}
	}
	data->id = best;
	data->width = gVesa_Modes[best].width;
	data->height = gVesa_Modes[best].height;
	data->bpp = gVesa_Modes[best].bpp;
	return best;
}

INT int Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data)
{
	if(data->id < 0 || data->id > giVesaModeCount)	return -1;
	data->width = gVesa_Modes[data->id].width;
	data->height = gVesa_Modes[data->id].height;
	data->bpp = gVesa_Modes[data->id].bpp;
	return 1;
}
