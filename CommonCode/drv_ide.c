/*
AcessOS 0.1
IDE Harddisk Controller
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_devfs2.h>
#include <drv_ide_int.h>

//DEFINES
#define	HDD_PRI_BASE	0x1F0
#define	HDD_SEC_BASE	0x170

#define IDE_AUTOMOUNT	1

#define IDE_DEBUG	0
#define DEBUG (IDE_DEBUG | ACESS_DEBUG)

/**
 \enum HddRegisters
 \brief Register offsets for different ports
*/
enum HddRegisters {
	HDD_DISK	= 0x06,
	HDD_CMD		= 0x07
};
/**
 \enum HddControls
 \brief Commands to be sent to HDD_CMD
*/
enum HddControls {
	LBA28_READ = 0x20,
	LBA28_WRITE = 0x20
};


//PROTOTYPES
static int hdd_ping(Uint8 disk);
static int hdd_read(Uint8 diskId, Uint address, Uint sectors, void *buffer);
static int hdd_write(Uint8 diskId, Uint address, Uint sectors, void *buffer);
void hdd_installFS();

//GLOBALS
t_disk_info	disk_layouts[4];
int	giCount[4] = {0,0,0,0};

//CODE
void setup_disks()
{
	int		part;
	int		culCount = 0;
	Uint8	disk;
	Uint8	bootSect[512];
	char	devName[32] = "/Devices/ide/";
	char	mountName[32] = "/Mount/hd";
	t_part_info	*tmpPart;
	
	for(disk=0;disk<4;disk++)
	{
		if(!hdd_ping(disk)) {
			continue;
		}
		
		if(!hdd_read(disk, 0, 1, &bootSect)) {
			puts("Error reading from disk\n");
			continue;
		}
		disk_layouts[disk].part_count = 0;
		
		//Update file names
		devName[13] = 'A'+(disk);
		devName[14] = '\0';
		//Print
		LogF("[IDE ] Disk %s\n", devName);
		for(part=0;part<4;part++)
		{
			tmpPart = &disk_layouts[disk].parts[part];
			if(bootSect[0x1BE + part*0x10 + 0x04] == 0x00) {
				tmpPart->fs = 0;
				continue;
			}
			tmpPart->fs = (Uint8) bootSect[0x1BE + part*0x10 + 0x04];
			tmpPart->lba_offset = (Uint) bootSect[0x1BE + part*0x10 + 0x08] << 0;
			tmpPart->lba_offset += (Uint) bootSect[0x1BE + part*0x10 + 0x09] << 8;
			tmpPart->lba_offset += (Uint) bootSect[0x1BE + part*0x10 + 0x0A] << 16;
			tmpPart->lba_offset += (Uint) bootSect[0x1BE + part*0x10 + 0x0B] << 24;
			
			tmpPart->lba_length = (Uint) bootSect[0x1BE + part*0x10 + 0x0C] << 0;
			tmpPart->lba_length += (Uint) bootSect[0x1BE + part*0x10 + 0x0D] << 8;
			tmpPart->lba_length += (Uint) bootSect[0x1BE + part*0x10 + 0x0E] << 16;
			tmpPart->lba_length += (Uint) bootSect[0x1BE + part*0x10 + 0x0F] << 24;
			
			//Update the file names
			mountName[10] = '1' + part;
			devName[14] = '1' + part;
			//Print
			LogF("[IDE ]  Partition %s\n", devName);
			
			//Increment Count - Needed for Automount
			disk_layouts[disk].part_count++;
		}
		culCount += disk_layouts[disk].part_count;
		giCount[disk] = culCount;
	}
	
	
	hdd_installFS();
	
	//Attempt Mount Device
	#if IDE_AUTOMOUNT
	for(disk=0;disk<4;disk++) {
		mountName[9] = 'a'+(disk);
		devName[13] = 'A'+(disk);
		mountName[10] = devName[14] = '\0';
		
		for(part=0;part<4;part++)
		{
			if(disk_layouts[disk].parts[part].fs == 0)
				continue;
			mountName[10] = '1' + part;
			devName[14] = '1' + part;
			
			vfs_mknod(mountName, 1);
			vfs_mount(devName, mountName, disk_layouts[disk].parts[part].fs);
		}
	}
	#endif
}

//
// hdd.c
//

/**
 \fn int hdd_ping(Uint8 disk)
 \param disk Disk to check
 \return Boolean
*/
static int hdd_ping(Uint8 disk)
{	
	switch(disk) {
	case 0x0:
		outportb(HDD_PRI_BASE+HDD_DISK, 0xA0);
		break;
	case 0x1:
		outportb(HDD_PRI_BASE+HDD_DISK, 0xB0);
		break;
	case 0x2:
		outportb(HDD_SEC_BASE+HDD_DISK, 0xA0);
		break;
	case 0x3:
		outportb(HDD_SEC_BASE+HDD_DISK, 0xB0);
		break;
	}

	//timer_wait(1);

	if(disk & 2) {
		if(inportb(HDD_SEC_BASE+HDD_CMD) & 0x40) {
			return 1;
		}
	} else {
		if(inportb(HDD_PRI_BASE+HDD_CMD) & 0x40) {
			return 1;
		}
	}
			
	return 0;
}

/**
 \fn static int hdd_read(Uint8 diskId, Uint address, Uint sectors, void *buffer)
 \brief Read data from the specified disk
 \param diskId Disk to read from
 \param address Offset of data to read (sectors)
 \param sectors Number of sectors to read
 \param buffer Buffer for data read
*/
static int hdd_read(Uint8 diskId, Uint address, Uint sectors, void *buffer)
{
	int		idx;
	unsigned short	tmpWord;
	unsigned short	base;
	Uint8	disk;
	unsigned short	*buf = buffer;
	
	#if DEBUG
	printf("hdd_read: (diskId=0x%x, address=0x%x, sectors=0x%x)\n", diskId, address, sectors);
	#endif
	
	//Get Controller
	if(diskId > 0x83)
		return 0;
	
	if(diskId & 2)
		base = HDD_SEC_BASE;
	else
		base = HDD_PRI_BASE;
	
	disk = diskId & 1;
	
	
	#if DEBUG
	printf(" hdd_read: base=0x%x, disk=%i\n", base, disk);
	#endif
	
	outportb(base+0x01, 0x00);
	outportb(base+0x02, (Uint8) sectors);		//Sector Count
	outportb(base+0x03, (Uint8) address);			//Low Addr
	outportb(base+0x04, (Uint8) (address >> 8));	//Middle Addr
	outportb(base+0x05, (Uint8) (address >> 16));	//Middle Addr
	outportb(base+0x06, 0xE0 | (disk << 4) | ((address >> 24) & 0x0F));	//Disk,Magic,High addr
	outportb(base+0x07, 0x20);	//Read Command
	
	while(!(inportb(base+0x07) & 0x08));	//Wait for read indicator
	
	for(idx=0;idx<256*sectors;idx++) {
		tmpWord = inportw(base+0x00);
		//printf("%x ", tmpWord);
		buf[idx] = tmpWord;
	}
	//printf("\n");
	return 1;
}

/**
 \fn int hdd_write(Uint8 diskId, Uint address, Uint sectors, void *buffer)
 \brief Write data to the specified disk
 \param diskId Disk to read from
 \param address Offset of data to write (sectors)
 \param sectors Number of sectors to write
 \param buffer Buffer for data write
*/
static int hdd_write(Uint8 diskId, Uint address, Uint sectors, void *buffer)
{
	int		idx;
	unsigned short	tmpword;
	unsigned short	base;
	Uint8	disk;
	unsigned short	*buf = buffer;
	
	//Get Controller
	if(diskId > 0x83)
		return 0;
	
	if(diskId & 2)
		base = HDD_SEC_BASE;
	else
		base = HDD_PRI_BASE;
	
	disk = diskId & 1;
	
	outportb(base+0x01, 0x00);
	outportb(base+0x02, (Uint8) sectors);
	outportb(base+0x03, (Uint8) address);			//Low Addr
	outportb(base+0x04, (Uint8) (address >> 8));	//Middle Addr
	outportb(base+0x05, (Uint8) (address >> 16));	//Middle Addr
	outportb(base+0x06, 0xE0 | (disk << 4) | ((address >> 24) & 0x0F));	//Disk,Magic,High addr
	outportb(base+0x07, 0x30);	//Read Command
	
	for(idx=0;idx<256*sectors;idx++)
	{
		tmpword = buf[idx];
		outportw(0x1F0, tmpword);
	}
	return 1;
}

//=============================================================================
//========================== fs_devfs2 interface ==============================
//=============================================================================

#define HDD_MAX_FS_NODES	16	//4 Disks / 4 Partitions

vfs_node *hdd_readdirFS(vfs_node *dirNode, int pos);
vfs_node *hdd_finddirFS(vfs_node *dirNode, char *filename);
int hdd_readFS(vfs_node *node, int off, int len, void *buffer);
int hdd_writeFS(vfs_node *node, int off, int len, void *buffer);
int hdd_ioctlFS(vfs_node *node, int id, void *data);

char	hdd_strbuf[HDD_MAX_FS_NODES*4];	//4 Bytes per Node
vfs_node	hdd_devNodes[HDD_MAX_FS_NODES];
devfs_driver	gHDD_DrvInfo = {
	{0}, hdd_ioctlFS
};
int			hdd_driver_id = -1;

/* Read Directory
 */
vfs_node *hdd_readdirFS(vfs_node *dirNode, int pos)
{
	if(pos >= gHDD_DrvInfo.rootNode.length)
		return NULL;
	return &hdd_devNodes[pos];
}

/* Find File
 */
vfs_node *hdd_finddirFS(vfs_node *dirNode, char *filename)
{
	int i;
	
	#if DEBUG
	printf("hdd_finddirFS: Open '%s'\n", filename);
	#endif
	
	for(i=0;i<HDD_MAX_FS_NODES;i++)
	{
		if(!hdd_devNodes[i].name)
			continue;
		if(filename[0] == hdd_devNodes[i].name[0] && filename[1] == '\0' && hdd_devNodes[i].name[1] == '\0')
			break;
		if(filename[0] == hdd_devNodes[i].name[0]
			&& filename[1] == hdd_devNodes[i].name[1]
			&& filename[2] == hdd_devNodes[i].name[2]
			&& hdd_devNodes[i].name[2] == '\0')
			break;
	}
	if(i == HDD_MAX_FS_NODES)
		return NULL;
	
	return &hdd_devNodes[ i ];
}

/* Read Data from file
 */
int hdd_readFS(vfs_node *node, int off, int len, void *buffer)
{
	char	*buf = buffer;
	int		devId = (node->inode >> 8) - 1;	//Disk is off By One
	int		partId = (node->inode & 0xFF) - 1;	//Same for partition
	
	if(node->inode == 0) {
		return -1;
	}
	
	#if DEBUG
	printf("hdd_readFS: (Handle:0x%x,Off:0x%x,Len:%i)\n", node->inode, off, len);
	printf(" hdd_readFS: Disk %i, Part %i\n", devId, partId);
	#endif
	
	if(partId != -1) {
		if(disk_layouts[devId].part_count <= partId)
			return -2;
		if(((off + len + 0x1FF) >> 9) > disk_layouts[devId].parts[partId].lba_length) {
			warning("Attempt to read from across filesystem boundary ((0x%x+0x%x+0x1FF)>>9 > %i)\n",
				off, len, disk_layouts[devId].parts[partId].lba_length);
			return -2;
		}
		off += disk_layouts[devId].parts[partId].lba_offset << 9;
	}
	
	if((off & 0x1FF) || (len & 0x1FF))
	{
		//Un-Aligned Offset/Length
		int		startOff = off>>9;
		int		sectOff = off&0x1FF;
		int		sectors = (len+0x1FF)>>9;
		int		i;
		Uint8	data[512];
	
		#if DEBUG
		printf("hdd_readFS: Non-aligned Read\n");
		#endif
		
		//Read Starting Sectors
		if(!hdd_read(devId, startOff, 1, data))
			return 0;
		memcpy(buf, (char*)(data+sectOff), len>512?512:len);
	
		if(len <= 512)	//If the data size is one sector or less
			return 1;	//Return
	
		//Read Middle Sectors
		for(i=1;i<sectors-1;i++)	{
			if(!hdd_read(devId, startOff+i, 1, data))
				return 0;
			memcpy((char*)(buf+i*512), data, 512);
		}
	
		//Read End Sectors
		if(!hdd_read(devId, startOff+i, 1, data))
			return 0;
		memcpy((char*)(buf+i*512), (char*)(data+sectOff), len-i*512);
		return 1;
	} else {
		#if DEBUG
		printf("hdd_readFS: Aligned Read\n");
		#endif
		//Aligned Offset and Length - Simple Code
		return hdd_read(devId|0x80, off >> 9, len >> 9, buffer);
	}
}

int hdd_writeFS(vfs_node *node, int off, int len, void *buffer)
{
	char	*buf = buf;
	int		devId = (node->inode >> 8) - 1;
	int		partId = (node->inode & 0xFF) - 1;	//Off by one
	
	if(node->inode == 0) {
		return -1;
	}
	
	if(partId != -1) {
		if(partId >= disk_layouts[devId].part_count)
			return -2;	
		if((off + len + 0x1FF) >> 9 > disk_layouts[devId].parts[partId].lba_length) {
			puts("ERROR: Attempt to write across filesystem boundary\n");
			return -2;
		}
		off += disk_layouts[devId].parts[partId].lba_offset << 9;
	}
	
	
	if(off & 0x1FF) {
		puts("ERROR: Offset is not on a sector boundary\n");
		return -1;
	}
	if(len & 0x1FF) {
		puts("ERROR: Length is not a whole number of sectors\n");
		return -1;
	}
	
	return hdd_write(devId|0x80, off >> 9, len >> 9, buffer);
}

/* IO Control
 */
int hdd_ioctlFS(vfs_node *node, int id, void *data)
{
	return 0;
}

void hdd_installFS()
{
	int i, disk, part;
	
	gHDD_DrvInfo.rootNode.name = "ide";		gHDD_DrvInfo.rootNode.nameLength = 3;
	gHDD_DrvInfo.rootNode.inode = 0;
	if(giCount[3] > 0)	gHDD_DrvInfo.rootNode.length = 4 + giCount[3];
	else if(giCount[2] > 0)	gHDD_DrvInfo.rootNode.length = 3 + giCount[2];
	else if(giCount[1] > 0)	gHDD_DrvInfo.rootNode.length = 2 + giCount[1];
	else if(giCount[0] > 0)	gHDD_DrvInfo.rootNode.length = 1 + giCount[0];
	gHDD_DrvInfo.rootNode.uid = 0;		gHDD_DrvInfo.rootNode.gid = 0;
	gHDD_DrvInfo.rootNode.mode = 0111;	//--X--X--X - Directory
	gHDD_DrvInfo.rootNode.flags = VFS_FFLAG_DIRECTORY;
	gHDD_DrvInfo.rootNode.ctime = gHDD_DrvInfo.rootNode.mtime = gHDD_DrvInfo.rootNode.atime = now();
	
	gHDD_DrvInfo.rootNode.read = NULL;	gHDD_DrvInfo.rootNode.write = NULL;
	gHDD_DrvInfo.rootNode.readdir = hdd_readdirFS;
	gHDD_DrvInfo.rootNode.finddir = hdd_finddirFS;
	gHDD_DrvInfo.rootNode.mknod = NULL;
	gHDD_DrvInfo.rootNode.unlink = NULL;
	gHDD_DrvInfo.rootNode.close = NULL;

	gHDD_DrvInfo.ioctl = hdd_ioctlFS;

	hdd_driver_id = dev_addDevice(&gHDD_DrvInfo);
	if(hdd_driver_id == -1) {
		panic("Unable to install IDE driver");
	}
	gHDD_DrvInfo.rootNode.impl = hdd_driver_id;
	
	for(i=0;i<gHDD_DrvInfo.rootNode.length;i++) {
		if(i < giCount[0]+1) {		//1st Disk
			hdd_strbuf[i*4] = 'A';
			disk = 0;	part = i-1;
			if(i!=0)	hdd_strbuf[i*4+1] = '1'+(i-1);
			else		hdd_strbuf[i*4+1] = '\0';
			hdd_strbuf[i*4+2] = '\0';
		}
		else if(i < giCount[1]+2) {		//2nd Disk
			disk = 1;	part = i-2-giCount[0];
			hdd_strbuf[i*4] = 'B';
			if(i!=giCount[0]+1)	hdd_strbuf[i*4+1] = '1'+(i-2-giCount[0]);
			else		hdd_strbuf[i*4+1] = '\0';
			hdd_strbuf[i*4+2] = '\0';
		}
		else if(i < giCount[2]+3) {		//3rd Disk
			disk = 2;	part = i-3-giCount[1];
			hdd_strbuf[i*4] = 'C';
			if(i!=giCount[1]+2)	hdd_strbuf[i*4+1] = '1'+(i-3-giCount[1]);
			else		hdd_strbuf[i*4+1] = '\0';
			hdd_strbuf[i*4+2] = '\0';
		}
		else if(i < giCount[3]+4) {		//4th Disk
			disk = 3;	part = i-4-giCount[2];
			hdd_strbuf[i*4] = 'D';
			if(i!=giCount[2]+3)	hdd_strbuf[i*4+1] = '1'+(i-4-giCount[2]);
			else		hdd_strbuf[i*4+1] = '\0';
			hdd_strbuf[i*4+2] = '\0';
		} else {
			warning("hdd_installFS - Too many disks");
			return;
		}
		
		hdd_devNodes[i].name = (char*)(hdd_strbuf+i*4);
		hdd_devNodes[i].nameLength = (hdd_devNodes[i].name[1]=='\0')?1:2;
		hdd_devNodes[i].impl = hdd_driver_id;	//Reqd by DevFS
		hdd_devNodes[i].inode = (hdd_devNodes[i].name[0]-'A'+1)<<8;
		hdd_devNodes[i].inode |= (hdd_devNodes[i].name[1]=='\0')?0:(hdd_devNodes[i].name[1]-'0');
		hdd_devNodes[i].length = disk_layouts[disk].parts[part].lba_length << 9;
		hdd_devNodes[i].uid = 0;	hdd_devNodes[i].gid = 0;
		hdd_devNodes[i].mode = 0664;
		
		hdd_devNodes[i].ctime = hdd_devNodes[i].mtime = hdd_devNodes[i].atime = now();
		hdd_devNodes[i].read = hdd_readFS;	hdd_devNodes[i].write = hdd_writeFS;
		hdd_devNodes[i].unlink = NULL;
		hdd_devNodes[i].readdir = NULL;	hdd_devNodes[i].finddir = NULL;
		hdd_devNodes[i].mknod = NULL;	hdd_devNodes[i].close = NULL;
	}
}
