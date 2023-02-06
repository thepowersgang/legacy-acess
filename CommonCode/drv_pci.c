/*
AcessOS/AcessBasic v0.1
PCI Bus Driver
*/
#include <acess.h>
#include <vfs.h>
#include <fs_devfs2.h>
#include <drv_pci.h>

#define DEBUG	0

// ==== STRUCTURES ====
typedef struct s_pciDevice {
	Uint16	bus, slot, fcn;
	Uint16	vendor, device;
	union {
		struct {Uint8 class, subclass;};
		Uint16	oc;
	};
	Uint16	revision;
	Uint32	ConfigCache[256/4];
} t_pciDevice;

// ==== CONSTANTS ====
#define SPACE_STEP	5
#define MAX_RESERVED_PORT	0xD00

// ==== GLOBAL DATA ====
 int	giPCI_BusCount = 1;
 int	giPCI_InodeHandle = -1;
 int	giPCI_DeviceCount = 0;
t_pciDevice	*gPCI_Devices = NULL;
devfs_driver	gPCI_DriverStruct = {
	{0}, 0
};
 Uint32	*gaPCI_PortBitmap = NULL;

// ==== PROTOTYPES ====
void	PCI_Install();
vfs_node	*PCI_ReadDirRoot(vfs_node *node, int pos);
vfs_node	*PCI_FindDirRoot(vfs_node *node, char *filename);
 int	PCI_CloseDevice(vfs_node *node);
 int	PCI_Ioctl(vfs_node *node, int id, void *data);
 int	PCI_EnumDevice(Uint16 bus, Uint16 dev, Uint16 fcn, t_pciDevice *info);
char	*PCI_int_MakeName(int id);
 int	PCI_ReadDevice(vfs_node *node, int pos, int length, void *buffer);
Uint32	PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
void	PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data);
Uint16	PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
Uint8	PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);

// ==== CODE ====
void PCI_Install()
{
	int bus, dev, fcn, i;
	int	space = 0;
	t_pciDevice	devInfo;
	void	*tmpPtr = NULL;
	
	// Build Portmap
	//LogF("[PCI ] Building Port Bitmap...");
	gaPCI_PortBitmap = malloc( 1 << 13 );
	memsetda( gaPCI_PortBitmap, 0, 1 << 11 );
	for( i = 0; i < MAX_RESERVED_PORT / 32; i ++ )
		gaPCI_PortBitmap[i] = -1;
	for( i = 0; i < MAX_RESERVED_PORT % 32; i ++ )
		gaPCI_PortBitmap[MAX_RESERVED_PORT / 32] = 1 << i;
	//LogF("Done.\n");
	
	// Scan Busses
	//LogF("[PCI ] Detecting PCI Devices...");
	for( bus = 0; bus < giPCI_BusCount; bus++ )
	{
		for( dev = 0; dev < 10; dev++ )	//10 Devices per bus
		{
			for( fcn = 0; fcn < 8; fcn++ )	// 8 functions per device
			{
				// Check if the device/function exists
				if(!PCI_EnumDevice(bus, dev, fcn, &devInfo))
				{
				//	LogF(" PCI %i,%i:%i NULL 0x%x\n", bus, dev, fcn, PCI_CfgReadDWord(bus, dev, fcn, 0));
					continue;
				}
				
				if(giPCI_DeviceCount == space)
				{
					space += SPACE_STEP;
					tmpPtr = realloc(space*sizeof(t_pciDevice), gPCI_Devices);
					if(tmpPtr == NULL)
						break;
					gPCI_Devices = tmpPtr;
				}
				if(devInfo.oc == PCI_OC_PCIBRIDGE)
				{
					LogF("[PCI ] Bridge @ %i,%i:%i (0x%x:0x%x)\n",
						bus, dev, fcn, devInfo.vendor, devInfo.device);
					giPCI_BusCount++;
				}
				memcpy(&gPCI_Devices[giPCI_DeviceCount], &devInfo, sizeof(t_pciDevice));
				giPCI_DeviceCount ++;
				LogF("[PCI ] Device %i,%i:%i => 0x%x:0x%x\n",
					bus, dev, fcn, devInfo.vendor, devInfo.device);
				
				// WTF is this for?
				if(fcn == 0) {
					if( !(devInfo.ConfigCache[3] & 0x800000) )
						break;
				}
			}
			if(tmpPtr != gPCI_Devices)
				break;
		}
		if(tmpPtr != gPCI_Devices)
			break;
	}
	tmpPtr = realloc(giPCI_DeviceCount*sizeof(t_pciDevice), gPCI_Devices);
	if(tmpPtr == NULL)
		return;
	gPCI_Devices = tmpPtr;
	//LogF("Done.\n");
	
	// Get Inode Handle
	giPCI_InodeHandle = inode_initCache();
	
	// Init Driver Struct
	gPCI_DriverStruct.ioctl = PCI_Ioctl;
	
	// Zero Structure
	memset(&gPCI_DriverStruct.rootNode, 0, sizeof(vfs_node));
	
	gPCI_DriverStruct.rootNode.name = "pci";
	gPCI_DriverStruct.rootNode.nameLength = 3;
	
	gPCI_DriverStruct.rootNode.readdir = PCI_ReadDirRoot;
	gPCI_DriverStruct.rootNode.finddir = PCI_FindDirRoot;
	
	gPCI_DriverStruct.rootNode.mode = 0111;	//--X--X--X
	gPCI_DriverStruct.rootNode.flags = VFS_FFLAG_DIRECTORY;
	
	gPCI_DriverStruct.rootNode.length = giPCI_DeviceCount;

	gPCI_DriverStruct.rootNode.inode = -1;
	gPCI_DriverStruct.rootNode.impl = dev_addDevice(&gPCI_DriverStruct);
}

char *PCI_int_MakeName(int id)
{
	char	*name;
	name = (char *) malloc(14);
	strcpy(name, "00.00:0");
	name[0] = '0' + gPCI_Devices[id].bus/10;
	name[1] = '0' + gPCI_Devices[id].bus%10;
	name[3] = '0' + gPCI_Devices[id].slot/10;
	name[4] = '0' + gPCI_Devices[id].slot%10;
	name[6] = '0' + gPCI_Devices[id].fcn;
	return name;
}

/*
vfs_node *PCI_ReadDirRoot(vfs_node *node, int pos)
- Read from Root of PCI Driver
*/
vfs_node *PCI_ReadDirRoot(vfs_node *node, int pos)
{
	vfs_node	*ret;
	
	if(pos < 0 || pos >= giPCI_DeviceCount)
		return NULL;
	ret = inode_getCache(giPCI_InodeHandle, pos);
	
	if(ret == NULL)
	{
		vfs_node	tmpNode;
		memset(&tmpNode, 0, sizeof(vfs_node));
		tmpNode.nameLength = 13;
		tmpNode.name = PCI_int_MakeName(pos);
		#if DEBUG
			LogF(" PCI_ReadDirRoot: tmpNode.name = '%s'\n", tmpNode.name);
		#endif
		tmpNode.mode = 0644;	tmpNode.flags = 0;
		tmpNode.length = 256;	tmpNode.inode = pos;
		
		tmpNode.read = PCI_ReadDevice;
		//tmpNode.write = PCI_WriteDevice;
		tmpNode.close = PCI_CloseDevice;
		return inode_cacheNode(giPCI_InodeHandle, pos, &tmpNode);
	}
	return ret;
}
vfs_node *PCI_FindDirRoot(vfs_node *node, char *filename)
{
	int bus,slot,fcn;
	int i;
	// Validate Filename (Pointer and length)
	if(!filename || strlen(filename) != 7)
		return NULL;
	// Check for spacers
	if(filename[2] != '.' || filename[5] != ':')
		return NULL;
	
	// Get Information
	if(filename[0] < '0' || filename[0] > '9')	return NULL;
	bus = (filename[0] - '0')*10;
	if(filename[1] < '0' || filename[1] > '9')	return NULL;
	bus += filename[1] - '0';
	if(filename[3] < '0' || filename[3] > '9')	return NULL;
	slot = (filename[3] - '0')*10;
	if(filename[4] < '0' || filename[4] > '9')	return NULL;
	slot += filename[4] - '0';
	if(filename[6] < '0' || filename[6] > '9')	return NULL;
	fcn = filename[6] - '0';
	
	// Find Match
	for(i=0;i<giPCI_DeviceCount;i++)
	{
		if(gPCI_Devices[i].bus != bus)		continue;
		if(gPCI_Devices[i].slot != slot)	continue;
		if(gPCI_Devices[i].fcn != fcn)	continue;
		
		return PCI_ReadDirRoot(node, i);
	}
	
	// Error Return
	return NULL;
}

int PCI_ReadDevice(vfs_node *node, int pos, int length, void *buffer)
{
	int i;
	t_pciDevice	*dev;
	char	*cfgCacheBytes;
	
	if( pos + length > 256 )	return 0;
	
	dev = &gPCI_Devices[ node->inode ];
	
	cfgCacheBytes = (char*)dev->ConfigCache;
	
	/*if(pos & 3)
	{*/
		for( i = 0; i < length; i++ )
			((char*)buffer)[i] = cfgCacheBytes[ pos + i ];
	/*} else {
		i = 0;
		while( !(length & 3) && length )
		{
			*((Uint32*)buffer+i) = dev->ConfigCache[(pos+i)/4];
			length -= 4;
			i += 4;
		}
		while(length);
		{
			((char*)buffer)[i] = cfgCacheBytes[ pos + i ];
			length--; i++;
		}
	}*/
	return length;
}

int PCI_CloseDevice(vfs_node *node)
{
	if(0 < node->inode || node->inode > giPCI_DeviceCount)
		return 0;
	free(node->name);
	inode_uncacheNode(giPCI_InodeHandle, node->inode);
	return 1;
}

int PCI_Ioctl(vfs_node *node, int id, void *data)
{
	return 0;
}

/**
 \fn int PCI_CountDevices(Uint16 vendor, Uint16 device, Uint16 fcn)
 \brief Counts the devices with the specified codes
 \param vendor	Vendor ID
 \param device	Device ID
 \param fcn	Function ID
*/
int PCI_CountDevices(Uint16 vendor, Uint16 device, Uint16 fcn)
{
	int i, ret=0;
	for(i=0;i<giPCI_DeviceCount;i++)
	{
		if(gPCI_Devices[i].vendor != vendor)	continue;
		if(gPCI_Devices[i].device != device)	continue;
		if(gPCI_Devices[i].fcn != fcn)	continue;
		ret ++;
	}
	return ret;
}

/**
 \fn int PCI_GetDevice(Uint16 vendor, Uint16 device, int idx)
 \brief Gets the ID of the specified PCI device
 \param vendor	Vendor ID
 \param device	Device ID
 \param fcn	Function IDs
 \param idx	Number of matching entry wanted
*/
int PCI_GetDevice(Uint16 vendor, Uint16 device, Uint16 fcn, int idx)
{
	int i, j=0;
	for(i=0;i<giPCI_DeviceCount;i++)
	{
		if(gPCI_Devices[i].vendor != vendor)	continue;
		if(gPCI_Devices[i].device != device)	continue;
		if(gPCI_Devices[i].fcn != fcn)	continue;
		if(j == idx)	return i;
		j ++;
	}
	return -1;
}

/**
 * \fn int PCI_GetDeviceByClass(Uint16 class, Uint16 mask, int prev)
 * \brief Gets the ID of a device by it's class code
 * \param class	Class Code
 * \param mask	Mask for class comparison
 * \param prev	ID of previous device (-1 for no previous)
 */
int PCI_GetDeviceByClass(Uint16 class, Uint16 mask, int prev)
{
	 int	i;
	// Check if prev is negative (meaning get first)
	if(prev < 0)	i = 0;
	else	i = prev+1;
	
	for( ; i < giPCI_DeviceCount; i++ )
	{
		if((gPCI_Devices[i].class & mask) != class)	continue;
		return i;
	}
	return -1;
}

/**
 \fn Uint8 PCI_GetIRQ(int id)
*/
Uint8 PCI_GetIRQ(int id)
{
	if(id < 0 || id >= giPCI_DeviceCount)
		return 0;
	return gPCI_Devices[id].ConfigCache[15];
	//return PCI_CfgReadByte( gPCI_Devices[id].bus, gPCI_Devices[id].slot, gPCI_Devices[id].fcn, 0x3C);
}

/**
 \fn Uint32 PCI_GetBAR0(int id)
*/
Uint32 PCI_GetBAR0(int id)
{
	if(id < 0 || id >= giPCI_DeviceCount)
		return 0;
	return gPCI_Devices[id].ConfigCache[4];
}

/**
 \fn Uint32 PCI_GetBAR1(int id)
*/
Uint32 PCI_GetBAR1(int id)
{
	if(id < 0 || id >= giPCI_DeviceCount)
		return 0;
	return gPCI_Devices[id].ConfigCache[5];
}

Uint16 PCI_AssignPort(int id, int bar, int count)
{
	Uint16	portVals;
	 int	gran=0;
	 int	i, j;
	t_pciDevice	*dev;
	
	//LogF("PCI_AssignPort: (id=%i,bar=%i,count=%i)\n", id, bar, count);
	
	if(id < 0 || id >= giPCI_DeviceCount)	return 0;
	if(bar < 0 || bar > 5)	return 0;
	
	dev = &gPCI_Devices[id];
	
	PCI_CfgWriteDWord( dev->bus, dev->slot,	dev->fcn, 0x10+bar*4, -1 );
	portVals = PCI_CfgReadDWord( dev->bus, dev->slot, dev->fcn, 0x10+bar*4 );
	dev->ConfigCache[4+bar] = portVals;
	//LogF(" PCI_AssignPort: portVals = 0x%x\n", portVals);
	
	// Check for IO port
	if( !(portVals & 1) )	return 0;
	
	// Mask out final bit
	portVals &= ~1;
	
	// Get Granuality
	__asm__ __volatile__ ("bsf %%eax, %%ecx" : "=c" (gran) : "a" (portVals) );
	gran = 1 << gran;
	//LogF(" PCI_AssignPort: gran = 0x%x\n", gran);
	
	// Find free space
	portVals = 0;
	for( i = 0; i < 1<<16; i += gran )
	{
		for( j = 0; j < count; j ++ )
		{
			if( gaPCI_PortBitmap[ (i+j)>>5 ] & 1 << ((i+j)&0x1F) )
				break;
		}
		if(j == count) {
			portVals = i;
			break;
		}
	}
	
	if(portVals)
	{
		for( j = 0; j < count; j ++ )
		{
			if( gaPCI_PortBitmap[ (portVals+j)>>5 ] |= 1 << ((portVals+j)&0x1F) )
				break;
		}
		PCI_CfgWriteDWord( dev->bus, dev->slot, dev->fcn, 0x10+bar*4, portVals|1 );
		dev->ConfigCache[4+bar] = portVals|1;
	}
	
	// Return
	//LogF("PCI_AssignPort: RETURN 0x%x\n", portVals);
	return portVals;
}


int	PCI_EnumDevice(Uint16 bus, Uint16 slot, Uint16 fcn, t_pciDevice *info)
{
	Uint16	vendor;
	 int	i;
	Uint32	addr;
	
	vendor = PCI_CfgReadWord(bus, slot, fcn, 0x0|0);
	if(vendor == 0xFFFF)	// Invalid Device
		return 0;
		
	info->bus = bus;
	info->slot = slot;
	info->fcn = fcn;
	info->vendor = vendor;
	info->device = PCI_CfgReadWord(bus, slot, fcn, 0x0|2);
	info->revision = PCI_CfgReadWord(bus, slot, fcn, 0x8|0);
	info->oc = PCI_CfgReadWord(bus, slot, fcn, 0x8|2);
	
	// Load Config Bytes
	addr = 0x80000000 | ((Uint)bus<<16) | ((Uint)slot<<11) | ((Uint)fcn<<8);
	for(i=0;i<256/4;i++)
	{
		outportd(0xCF8, addr);
		info->ConfigCache[i] = inportd(0xCFC);
		addr += 4;
	}
	
	return 1;
}

Uint32 PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	address;
	Uint32	data;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 5 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	address = 0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	outportd(0xCF8, address);
	
	data = inportd(0xCFC);
	return (Uint16)data;
}
void PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data)
{
	Uint32	address;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 5 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	address = 0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	outportd(0xCF8, address);
	outportd(0xCFC, data);
}
Uint16 PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	data;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 5 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	//LogF("PCI_CfgReadWord: (bus=0x%x,dev=0x%x,func=%x,offset=0x%x)\n", bus, dev, func, offset);
	
	outportd(0xCF8,
		0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC) );
	
	data = inportd(0xCFC);
	data >>= (offset&2)*8;	//Allow Access to Upper Word
	//LogF("PCI_CfgReadWord: RETURN 0x%x\n", data&0xFFFF);
	return (Uint16)data;
}

Uint8 PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	address;
	Uint32	data;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 4 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	address = 0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	outportd(0xCF8, address);
	
	data = inportd(0xCFC);
	data >>= (offset&3)*8;	//Allow Access to Upper Word
	return (Uint8)data;
}

