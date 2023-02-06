/*
Acess OS
FAT16 Driver Version 1
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_fat_int.h>

#define FAT_DEBUG	0
#define DEBUG (FAT_DEBUG | ACESS_DEBUG)

#if DEBUG
# define DEBUGS LogF
#else
# define DEBUGS(v...)
#endif

#define CACHE_FAT	1
#define USE_LFN		1

//SEMI-GLOBALS
drv_fat_volinfo	fat_disks[8];
int		partCount = 0;
#if CACHE_FAT
Uint32	*fat_cache[8];
#endif
#if USE_LFN
typedef struct s_lfncache {
	Uint	Inode, Impl;
	 int	id;
	char	Name[256];
	struct s_lfncache	*Next;
}	t_lfncache;
t_lfncache	*fat_lfncache;
#endif

//PROTOTYPES
vfs_node	*FAT_InitDevice(char *device);
 int	FAT_CloseDevice(vfs_node *node);
 int	FAT_Read(vfs_node *node, int offset, int length, void *buffer);
 int	FAT_Write(vfs_node *node, int offset, int length, void *buffer);
vfs_node	*FAT_ReadDir(vfs_node *dirNode, int dirpos);
vfs_node	*FAT_FindDir(vfs_node *dirNode, char *file);
vfs_node	*FAT_Mknod(vfs_node *node, char *name, int isdir);
 int	FAT_Unlink(vfs_node *node);
 int	FAT_CloseFile(vfs_node *node);

//CODE
/* Reads the boot sector of a disk and prepares the structures for it
 */
vfs_node *FAT_InitDevice(char *device)
{
	fat_bootsect *bs;
	int	i;
	Uint32	FATSz, RootDirSectors, TotSec, CountofClusters;
	vfs_node	*node = NULL;
	drv_fat_volinfo	*diskInfo = &fat_disks[partCount];
	
	//Temporary Pointer
	bs = &diskInfo->bootsect;
	
	#if DEBUG
	LogF("FAT_InitDisk: (device='%s')\n", device);
	#endif
	//Open device and read boot sector
	diskInfo->fileHandle = vfs_open(device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	if(diskInfo->fileHandle == -1) {
		LogF("FAT_InitDisk: Unable to open device.\n");
		return NULL;
	}
	#if DEBUG
	LogF(" FAT_InitDisk: Device open (Handle #%u)\n", diskInfo->fileHandle);
	#endif
	vfs_read(diskInfo->fileHandle, 512, bs);
	#if DEBUG
	LogF(" FAT_InitDisk: Boot Sector Read\n");
	#endif
	
	if(bs->bps == 0 || bs->spc == 0) {
		LogF(" FAT_InitDisk: Error in FAT Boot Sector\n");
		return NULL;
	}
	
	#if DEBUG
	LogF(" FAT_InitDisk: spc = %i\n", bs->spc);
	LogF(" FAT_InitDisk: bps = %i\n", bs->bps);
	LogF(" FAT_InitDisk: OEM Name: '%c%c%c%c%c%c%c%c'\n",
		bs->oem_name[0], bs->oem_name[1], bs->oem_name[2], bs->oem_name[3],
		bs->oem_name[4], bs->oem_name[5], bs->oem_name[6], bs->oem_name[7]);
	#endif
	
	//FAT Type Determining
	// From Microsoft FAT Specifcation
	RootDirSectors = ((bs->files_in_root*32) + (bs->bps - 1)) / bs->bps;
	
	if(bs->fatSz16 != 0)		FATSz = bs->fatSz16;
	else					FATSz = bs->spec.fat32.fatSz32;
	
	if(bs->totalSect16 != 0)		TotSec = bs->totalSect16;
	else						TotSec = bs->totalSect32;
	
	CountofClusters = (TotSec - (bs->resvSectCount + (bs->fatCount * FATSz) + RootDirSectors)) / bs->spc;
	
	LogF("[FAT ] '%s' ", device);
	if(CountofClusters < 4085) {
		diskInfo->type = FAT12;
		puts("FAT12");
	} else if(CountofClusters < 65525) {
		diskInfo->type = FAT16;
		puts("FAT16");
	} else {
		diskInfo->type = FAT32;
		puts("FAT32");
	}
	{
		int kbytes = CountofClusters * bs->spc / 2;
		if(kbytes <= 2*1024)
			LogF(", %i KiB\n", kbytes);
		else if(kbytes <= 2*1024*1024)
			LogF(", %i MiB\n", kbytes>>10);
		else
			LogF(", %i GiB\n", kbytes>>20);
	}
	
	//Get Name
	//puts(" Name: ");
	if(diskInfo->type == FAT32) {
		for(i=0;i<11;i++)
			diskInfo->name[i] = (bs->spec.fat32.label[i] == ' ' ? '\0' : bs->spec.fat32.label[i]);
	}
	else {
		for(i=0;i<11;i++)
			diskInfo->name[i] = (bs->spec.fat16.label[i] == ' ' ? '\0' : bs->spec.fat16.label[i]);
	}
	diskInfo->name[12] = '\0';
	//puts(diskInfo->name); putch('\n');
	
	//Compute Root directory offset
	if(diskInfo->type == FAT32)
		diskInfo->rootOffset = bs->spec.fat32.rootClust;
	else
		diskInfo->rootOffset = (FATSz * bs->fatCount) / bs->spc;
	
	#if DEBUG
	LogF(" FAT_InitDisk: Root Offset = %i clusters\n", (Uint)diskInfo->rootOffset);
	#endif
	
	diskInfo->clusterCount = CountofClusters;
	
	diskInfo->firstDataSect = bs->resvSectCount + (bs->fatCount * FATSz) + RootDirSectors;
	
	//Allow for Caching the FAT
	#if CACHE_FAT
	# if DEBUG
		LogF(" FAT_InitDisk: Caching FAT\n");
	# endif
	fat_cache[partCount] = (Uint32*)malloc(sizeof(Uint32)*CountofClusters);
	if(fat_cache[partCount] == NULL) {
		panic("Out of memory (FAT_InitDisk)\n");
		return NULL;
	}
	vfs_seek(diskInfo->fileHandle, bs->resvSectCount*512, SEEK_SET);
	if(diskInfo->type == FAT12) {
		Uint32	val;
		int		j;
		char	buf[1536];
		vfs_read(diskInfo->fileHandle, 512*3, buf);
		for(i=0;i<CountofClusters/2;i++) {
			j = i & 511;	//%512
			val = *((int*)(buf+j*3));
			fat_cache[partCount][i*2] = val & 0xFFF;
			fat_cache[partCount][i*2+1] = (val>>12) & 0xFFF;
			if(j == 511) {
				vfs_read(diskInfo->fileHandle, 512*3, buf);
			}
		}
	}
	if(diskInfo->type == FAT16) {
		Uint16	buf[256];
		vfs_read(diskInfo->fileHandle, 512, buf);
		for(i=0;i<CountofClusters;i++) {
			fat_cache[partCount][i] = buf[i&255];
			if((i&255) == 255) {
				vfs_read(diskInfo->fileHandle, 512, buf);
			}
		}
	}
	if(diskInfo->type == FAT32) {
		Uint32	buf[128];
		vfs_read(diskInfo->fileHandle, 512, buf);
		for(i=0;i<CountofClusters;i++) {
			fat_cache[partCount][i] = buf[i&127];
			if((i&127) == 127) {
				vfs_read(diskInfo->fileHandle, 512, buf);
			}
		}
	}
	DEBUGS(" FAT_InitDisk: FAT Fully Cached\n");
	#endif /*CACHE_FAT*/
	
	//Initalise inode cache for FAT
	fat_disks[partCount].inodeHandle = inode_initCache();
	
	#if DEBUG
		LogF(" FAT_InitDisk: Inode Cache handle is %i\n", fat_disks[partCount].inodeHandle);
	#endif
	
	//== VFS2 Interface
	node = &fat_disks[partCount].rootNode;
	node->name = fat_disks[partCount].name;	node->nameLength = strlen(node->name);
	node->inode = diskInfo->rootOffset;
	node->length = bs->files_in_root;	//Unknown - To be set on readdir
	node->impl = partCount;
	
	node->uid = 0;	node->gid = 0;	node->mode = 0777;
	node->flags = VFS_FFLAG_DIRECTORY;
	node->ctime = node->mtime = node->atime = now();
	
	node->read = node->write = NULL;
	node->readdir = FAT_ReadDir;
	node->finddir = FAT_FindDir;
	node->mknod = FAT_Mknod;
	node->close = NULL;
	node->unlink = FAT_CloseDevice;
	
	partCount++;
	return node;
}

/* Closes a mount and marks it as free
 */
int FAT_CloseDevice(vfs_node *node)
{
	vfs_close(fat_disks[node->impl].fileHandle);
	inode_clearCache(fat_disks[node->impl].inodeHandle);
	fat_disks[node->impl].fileHandle = -2;
	return 1;
}

/* Fetches a value from the FAT
 */
static Uint32 FAT_int_GetFatValue(int handle, Uint32 cluster)
{
	Uint32	val;
	#if CACHE_FAT
	val = fat_cache[handle][cluster];
	#else
	if(fat_disks[handle].type == FAT12) {
		vfs_seek(fat_disks[handle].fileHandle, 512+(cluster&0xFFFFE)*3, SEEK_SET);
		vfs_read(fat_disks[handle].fileHandle, 3, &val);
		val = (cluster&1 ? val&0xFFF : val>>12);
	} else if(fat_disks[handle].type == FAT16) {
		vfs_seek(fat_disks[handle].fileHandle, 512+cluster*2, SEEK_SET);
		vfs_read(fat_disks[handle].fileHandle, 2, &val);
	} else {
		vfs_seek(fat_disks[handle].fileHandle, 512+cluster*4, SEEK_SET);
		vfs_read(fat_disks[handle].fileHandle, 4, &val);
	}
	#endif /*CACHE_FAT*/
	DEBUGS(" fat_int_getValue: val=0x%x (%u)\n", (Uint)val, (Uint)val);
	return val;
}

/* Reads a cluster's data
 */
static void FAT_int_ReadCluster(int handle, Uint32 cluster, int length, void *buffer)
{
	#if DEBUG
	LogF("FAT_int_ReadCluster: (handle=%i, cluster=0x%x, length=%i)\n", handle, (Uint)cluster, length);
	#endif
	vfs_seek(
		fat_disks[handle].fileHandle, 
		(fat_disks[handle].firstDataSect + (cluster-2)*fat_disks[handle].bootsect.spc )
			* fat_disks[handle].bootsect.bps,
		SEEK_SET);
	vfs_read(fat_disks[handle].fileHandle, length, buffer);
}

/* Reads data from a specified file
 */
int FAT_Read(vfs_node *node, int offset, int length, void *buffer)
{
	int preSkip, count;
	int handle = node->impl;
	int i, cluster, pos;
	int	bpc;
	void	*tmpBuf;
	Uint	eocMarker;
	
	#if DEBUG
		LogF("FAT_Read: (offset=%i,length=%i,buffer=0x%x)\n", offset, length, buffer);
	#endif
	
	// Calculate and Allocate Bytes Per Cluster
	bpc = fat_disks[node->impl].bootsect.spc * fat_disks[node->impl].bootsect.bps;
	tmpBuf = (void*) malloc(bpc);
	
	// Cluster is stored in Inode Field
	cluster = node->inode;
	
	// Get EOC Marker
	if(fat_disks[node->impl].type == FAT12)	eocMarker = EOC_FAT12;
	else if(fat_disks[node->impl].type == FAT16)	eocMarker = EOC_FAT16;
	else if(fat_disks[node->impl].type == FAT32) eocMarker = EOC_FAT32;
	else {
		LogF("ERROR: Unsupported FAT Variant.\n");
		return 0;
	}
	
	// Single Cluster including offset
	if(length + offset < bpc)
	{
		FAT_int_ReadCluster(handle, cluster, bpc, tmpBuf);
		memcpy( buffer, (void*)( tmpBuf + offset%bpc ), length );
		free(tmpBuf);
		return 1;
	}
	
	preSkip = offset / bpc;
	
	//Skip previous clusters
	for(i=preSkip;i--;)	{
		cluster = FAT_int_GetFatValue(handle, cluster);
		if(cluster == eocMarker) {
			warning("FAT_Read - Offset is past end of cluster chain mark\n");
		}
	}
	
	// Get Count of Clusters to read
	count = ((offset%bpc+length) / bpc) + 1;
	
	// Get buffer Position after 1st cluster
	pos = bpc - offset%bpc;
	
	// Read 1st Cluster
	FAT_int_ReadCluster(handle, cluster, bpc, tmpBuf);
	memcpy(
		buffer,
		(void*)( tmpBuf + (bpc-pos) ),
		(pos < length ? pos : length)
		);
	
	if (count == 1) {
		free(tmpBuf);
		return 1;
	}
	
	cluster = FAT_int_GetFatValue(handle, cluster);
	
	#if DEBUG
		LogF(" FAT_Read: pos=%i\n", pos);
		LogF(" FAT_Read: Reading the rest of the clusters\n");
	#endif
	
	
	//Read the rest of the cluster data
	for(i=1;i<count-1;i++) {
		FAT_int_ReadCluster(handle, cluster, bpc, tmpBuf);
		memcpy((void*)(buffer+pos), tmpBuf, bpc);
		pos += bpc;
		cluster = FAT_int_GetFatValue(handle, cluster);
		if(cluster == eocMarker) {
			warning("FAT_Read - Read past End of Cluster Chain\n");
			free(tmpBuf);
			return 0;
		}
	}
	
	FAT_int_ReadCluster(handle, cluster, bpc, tmpBuf);
	memcpy((void*)(buffer+pos), tmpBuf, length-pos);
	
	#if DEBUG
		LogF(" FAT_Read: Free tmpBuf(0x%x) and Return\n", tmpBuf);
	#endif
	
	free(tmpBuf);
	return 1;
}


int FAT_Write(vfs_node *node, int offset, int length, void *buffer)
{
	return 0;
}

/* Converts a FAT directory entry name into a proper filename
 */
static void FAT_int_ProperFilename(char *dest, char *src)
{
	int a, b;
	
	for(a=0;a<8;a++) {
		if(src[a] == ' ')	break;
		dest[a] = src[a];
	}
	b = a;
	a = 8;
	if(src[8] != ' ')	dest[b++] = '.';
	for(;a<11;a++,b++)	{
		if(src[a] == ' ')	break;
		dest[b] = src[a];
	}
	dest[b] = '\0';
	#if DEBUG
	LogF("FAT_int_ProperFilename: dest='%s'\n", dest);
	#endif
}

/* Creates a vfs_node structure for a given file entry
 */
vfs_node *FAT_int_CreateNode(vfs_node *parent, fat_filetable *ft, char *LongFileName)
{
	vfs_node node;
	#if USE_LFN
	int	len;
	#endif
	
	#if DEBUG
	LogF("FAT_int_CreateNode: (parent={impl=%i,inode=0x%x})\n", (Uint)parent->impl, (Uint)parent->inode);
	#endif
	
	//Get Name
	#if USE_LFN
	if(LongFileName && LongFileName[0] != '\0')
	{	
		len = strlen(LongFileName);
		node.name = malloc(len+1);
		strcpy(node.name, LongFileName);
		node.nameLength = len;
	}
	else
	{
	#endif
		node.name = (char*) malloc(13);
		memset(node.name, 13, '\0');
		FAT_int_ProperFilename(node.name, ft->name);
		node.nameLength = strlen(node.name);
	#if USE_LFN
	}
	#endif
	//Set Other Data
	node.inode = ft->cluster | (ft->clusterHi<<16);
	node.length = ft->size;
	//LogF("File Size = 0x%x\n", node.length);
	node.impl = parent->impl;
	node.uid = 0;	node.gid = 0;	node.mode = 0775;	//RWXRWXR-X
	
	node.flags = 0;
	if(ft->attrib & ATTR_DIRECTORY)	node.flags |= VFS_FFLAG_DIRECTORY;
	if(ft->attrib & ATTR_READONLY)	node.flags |= VFS_FFLAG_READONLY;
	
	node.atime = timestamp(0,0,0,
			((ft->adate&0x1F)-1),	//Days
			((ft->adate&0x1E0)-1),		//Months
			1980+((ft->adate&0xFF00)>>8));	//Years
	
	node.ctime = ft->ctimems / 100;	//Miliseconds
	node.ctime += timestamp((ft->ctime&0x1F)<<1,	//Seconds
			((ft->ctime&0x3F0)>>5),	//Minutes
			((ft->ctime&0xF800)>>11),	//Hours
			((ft->cdate&0x1F)-1),		//Days
			((ft->cdate&0x1E0)-1),		//Months
			1980+((ft->cdate&0xFF00)>>8));	//Years
			
	node.mtime = timestamp((ft->mtime&0x1F)<<1,	//Seconds
			((ft->mtime&0x3F0)>>5),	//Minuites
			((ft->mtime&0xF800)>>11),	//Hours
			((ft->mdate&0x1F)-1),		//Days
			((ft->mdate&0x1E0)-1),		//Months
			1980+((ft->mdate&0xFF00)>>8));	//Years
	
	if(node.flags & VFS_FFLAG_DIRECTORY) {
		node.readdir = FAT_ReadDir;
		node.finddir = FAT_FindDir;
		node.mknod = FAT_Mknod;
	} else {
		node.read = FAT_Read;
		node.write = FAT_Write;
	}
	node.close = FAT_CloseFile;
	node.unlink = FAT_Unlink;
	
	#if DEBUG
	LogF(" FAT_int_CreateNode: Creating new node\n");
	#endif
	return inode_cacheNode(fat_disks[parent->impl].inodeHandle, node.inode, &node);
}

#if USE_LFN
/**
 \fn char *FAT_int_GetLFN(vfs_node *node)
 \brief Return pointer to LFN cache entry
 */
char *FAT_int_GetLFN(vfs_node *node)
{
	t_lfncache	*tmp;
	tmp = fat_lfncache;
	while(tmp)
	{
		if(tmp->Inode == node->inode && tmp->Impl == node->impl)
			return tmp->Name;
		tmp = tmp->Next;
	}
	tmp = malloc(sizeof(t_lfncache));
	tmp->Inode = node->inode;
	tmp->Impl = node->impl;
	memsetda(tmp->Name, 0, 256/4);
	
	tmp->Next = fat_lfncache;
	fat_lfncache = tmp;
	
	return tmp->Name;
}

/**
 \fn void FAT_int_DelLFN(vfs_node *node)
 \brief Delete a LFN cache entry
*/
void FAT_int_DelLFN(vfs_node *node)
{
	t_lfncache	*tmp;
	if(!fat_lfncache)	return;
	if(!fat_lfncache->Next)
	{
		tmp = fat_lfncache;
		fat_lfncache = tmp->Next;
		free(tmp);
		return;
	}
	tmp = fat_lfncache;
	while(tmp && tmp->Next)
	{
		if(tmp->Inode == node->inode && tmp->Impl == node->impl)
		{
			free(tmp->Next);
			tmp->Next = tmp->Next->Next;
			return;
		}
		tmp = tmp->Next;
	}
}
#endif

/**
 \fn vfs_node *FAT_ReadDir(vfs_node *dirNode, int dirPos)
 \param dirNode	Node structure of directory
 \param dirPos	Directory position
**/
vfs_node *FAT_ReadDir(vfs_node *dirNode, int dirpos)
{
	fat_filetable	fileinfo[16];	//Sizeof=32, 16 per sector
	 int	a=0;
	drv_fat_volinfo	*disk = &fat_disks[dirNode->impl&7];
	Uint32	cluster, offset;
	 int	preSkip;
	vfs_node	*node;
	#if USE_LFN
	char	*lfn = NULL;
	#endif
	
	#if DEBUG
	LogF("FAT_ReadDir: (dirNode(0x%x)={inode=%i, impl=%i}, dirPos=%i)\n", 
		dirNode, (Uint)dirNode->inode, (Uint)dirNode->impl, dirpos);
	#endif
	
	// Get Byte Offset and skip
	offset = dirpos * sizeof(fat_filetable);
	preSkip = (offset >> 9) / disk->bootsect.spc;	// >>9 == /512
	cluster = dirNode->inode;	// Cluster ID
	
	// Do Cluster Skip
	// - Pre FAT32 had a reserved area for the root.
	if( !(disk->type != FAT32 && cluster == disk->rootOffset) )
	{
		//Skip previous clusters
		for(a=preSkip;a--;)	{
			cluster = FAT_int_GetFatValue(dirNode->impl, cluster);
		}
	}
	
	// Check for end of cluster chain
	if((disk->type == FAT12 && cluster == EOC_FAT12)
	|| (disk->type == FAT16 && cluster == EOC_FAT16)
	|| (disk->type == FAT32 && cluster == EOC_FAT32))
		return NULL;
	
	// Bounds Checking (Used to spot heap overflows)
	if(cluster > disk->clusterCount + 2)
	{
		warning("FAT_ReadDir - Cluster ID is over cluster count (0x%x>0x%x)\n",
			cluster, disk->clusterCount+2);
		return NULL;
	}
	
	#if DEBUG
		LogF(" FAT_ReadDir: cluster=0x%x, dirpos=%i\n", cluster, dirpos);
	#endif
	
	// Compute Offsets
	// - Pre FAT32 cluster base (in sectors)
	if( cluster == disk->rootOffset && disk->type != FAT32 )
		offset = disk->bootsect.resvSectCount + cluster*disk->bootsect.spc;
	else
	{	// FAT32 cluster base (in sectors)
		offset = disk->firstDataSect;
		offset += (cluster - 2) * disk->bootsect.spc;
	}
	// Sector in cluster
	if(disk->bootsect.spc == 1)
		offset += (dirpos / 16);
	else
		offset += (dirpos / 16) % disk->bootsect.spc;
	// Offset in sector
	a = dirpos & 0xF;
	
	#if DEBUG
		LogF(" FAT_ReadDir: offset=%i, a=%i\n", (Uint)offset, a);
	#endif
	
	// Read Sector
	vfs_seek(disk->fileHandle, offset*512, 1);	//Seek Set
	vfs_read(disk->fileHandle, 512, fileinfo);	//Read Dir Data
	
	DEBUGS(" FAT_ReadDir: name[0] = 0x%x\n", (Uint8)fileinfo[a].name[0]);
	//Check if this is the last entry
	if(fileinfo[a].name[0] == '\0') {
		dirNode->length = dirpos;
		DEBUGS(" FAT_ReadDir: End of list\n");
		return NULL;	// break
	}
	
	// Check for empty entry
	if((Uint8)fileinfo[a].name[0] == 0xE5) {
		DEBUGS(" FAT_ReadDir: Empty Entry\n");
		return (vfs_node*)1;	// Skip
	}
	
	#if USE_LFN
	// Get Long File Name Cache
	lfn = FAT_int_GetLFN(dirNode);
	if(fileinfo[a].attrib == ATTR_LFN)
	{
		fat_longfilename	*lfnInfo;
		 int	len;
		
		lfnInfo = (fat_longfilename *) &fileinfo[a];
		if(lfnInfo->id & 0x40)	memsetda(lfn, 0, 256/4);
		//else {
		//	warning("FAT_ReadDir - Long Filename block starting without 0x40 set");
		//	return 0;
		//}
		
		//while(fileinfo[a&15].attrib == ATTR_LFN)
		//{
			// If it's the first entry, clear the cache
			//if(lfnInfo->id & 0x40)	memsetda(lfn, 0, 256/4);
			// Get the current length
			len = strlen(lfn);
			
			// Sanity Check (FAT implementations do not allow >255 bytes)
			if(len + 13 > 255)	return &NULLNode;
			// Rebase all bytes
			for(a=len+1;a--;)	lfn[a+13] = lfn[a];
			
			// Append new bytes
			lfn[ 0] = lfnInfo->name1[0];	lfn[ 1] = lfnInfo->name1[1];
			lfn[ 2] = lfnInfo->name1[2];	lfn[ 3] = lfnInfo->name1[3];
			lfn[ 4] = lfnInfo->name1[4];	
			lfn[ 5] = lfnInfo->name2[0];	lfn[ 6] = lfnInfo->name2[1];
			lfn[ 7] = lfnInfo->name2[2];	lfn[ 8] = lfnInfo->name2[3];
			lfn[ 9] = lfnInfo->name2[4];	lfn[10] = lfnInfo->name2[5];
			lfn[11] = lfnInfo->name3[0];	lfn[12] = lfnInfo->name3[1];
			//a ++;
		//}
		return &NULLNode;
	}
	#endif
	
	//Check if it is a volume entry
	if(fileinfo[a].attrib & 0x08) {
		return &NULLNode;
	}
	// Ignore . and ..
	if(fileinfo[a].name[0] == '.')
		return &NULLNode;
	
	
	// Check if Node is cached
	cluster = fileinfo[a].cluster | (fileinfo[a].clusterHi << 16);
	node = inode_getCache(disk->inodeHandle, cluster);
	if(node != NULL)	//Use Inode Cache
		return node;
	
	
	#if DEBUG
		LogF(" FAT_ReadDir: name='%c%c%c%c%c%c%c%c%c%c%c'\n",
			fileinfo[a].name[0], fileinfo[a].name[1], fileinfo[a].name[2], fileinfo[a].name[3],
			fileinfo[a].name[4], fileinfo[a].name[5], fileinfo[a].name[6], fileinfo[a].name[7],
			fileinfo[a].ext [0], fileinfo[a].ext [1], fileinfo[a].ext [2] );
	#endif
	
	#if USE_LFN
	node = FAT_int_CreateNode(dirNode, &fileinfo[a], lfn);
	lfn[0] = '\0';
	#else
	node = FAT_int_CreateNode(dirNode, &fileinfo[a], NULL);
	#endif
	
	#if DEBUG
		LogF("FAT_ReadDir: RETURN 0x%x\n", node);
	#endif
	return node;
}

/* Finds an entry in the current directory
 */
vfs_node *FAT_FindDir(vfs_node *node, char *name)
{
	fat_filetable	fileinfo[32];
	char	tmpName[11];
	#if USE_LFN
	fat_longfilename	*lfnInfo;
	char	*lfn = NULL;
	int		lfnPos=255, lfnId = -1;
	#endif
	int 	i=0;
	vfs_node	*tmpNode;
	drv_fat_volinfo	*disk = &fat_disks[node->impl];
	Uint32	dirCluster;
	Uint32	cluster;
	
	#if DEBUG
	LogF("FAT_FindDir: (node=0x%x, name='%s')\n", node, name);
	#endif
	
	// Fast Returns
	if(!name)	return NULL;
	if(name[0] == '\0')	return NULL;
	
	#if USE_LFN
	lfn = FAT_int_GetLFN(node);
	#endif
	
	dirCluster = node->inode;
	//Seek to Directory
	if( dirCluster == disk->rootOffset && disk->type != FAT32 )
		vfs_seek(disk->fileHandle, (disk->bootsect.resvSectCount+dirCluster*disk->bootsect.spc)<<9, SEEK_SET);
	else
		vfs_seek(disk->fileHandle, (disk->firstDataSect+(dirCluster-2)*disk->bootsect.spc)<<9, SEEK_SET);
	for(;;i++)
	{
		//Load sector
		if((i & 0xF) == 0) {
			vfs_read(disk->fileHandle, 512, fileinfo);
		}
		
		//Check if the files are free
		if(fileinfo[i&0xF].name[0] == '\0')	break;		//Free and last
		if(fileinfo[i&0xF].name[0] == '\xE5')	goto loadCluster;	//Free
		
		
		#if USE_LFN
		// Long File Name Entry
		if(fileinfo[i&0xF].attrib == ATTR_LFN)
		{
			lfnInfo = (fat_longfilename *) &fileinfo[i&0xF];
			if(lfnInfo->id & 0x40) {
				memset(lfn, 0, 256);
				lfnPos = 255;
			}
			lfn[lfnPos--] = lfnInfo->name3[1];	lfn[lfnPos--] = lfnInfo->name3[0];
			lfn[lfnPos--] = lfnInfo->name2[5];	lfn[lfnPos--] = lfnInfo->name2[4];
			lfn[lfnPos--] = lfnInfo->name2[3];	lfn[lfnPos--] = lfnInfo->name2[2];
			lfn[lfnPos--] = lfnInfo->name2[1];	lfn[lfnPos--] = lfnInfo->name2[0];
			lfn[lfnPos--] = lfnInfo->name1[4];	lfn[lfnPos--] = lfnInfo->name1[3];
			lfn[lfnPos--] = lfnInfo->name1[2];	lfn[lfnPos--] = lfnInfo->name1[1];
			lfn[lfnPos--] = lfnInfo->name1[0];
			if((lfnInfo->id&0x3F) == 1)
			{
				memcpy(lfn, lfn+lfnPos+1, 256-lfnPos);
				lfnId = i+1;
			}
		}
		else
		{
			// Remove LFN if it does not apply
			if(lfnId != i)	lfn[0] = '\0';
		#endif
			// Get Real Filename
			FAT_int_ProperFilename(tmpName, fileinfo[i&0xF].name);
		
			//Only Long name is case sensitive, 8.3 is not
			#if USE_LFN
			if(strncmp(tmpName, name) == 0 || strcmp(lfn, name) == 0) {
			#else
			if(strncmp(tmpName, name) == 0) {
			#endif
				cluster = fileinfo[i&0xF].cluster | (fileinfo[i&0xF].clusterHi << 16);
				tmpNode = inode_getCache(disk->inodeHandle, cluster);
				if(tmpNode == NULL)	// Node is not cached
				{
					#if USE_LFN
					tmpNode = FAT_int_CreateNode(node, &fileinfo[i&0xF], lfn);
					#else
					tmpNode = FAT_int_CreateNode(node, &fileinfo[i&0xF], NULL);
					#endif
				}
				#if DEBUG
				LogF("FAT_FindDir: RETURN 0x%x\n", tmpNode);
				#endif
				#if USE_LFN
				lfn[0] = '\0';
				#endif
				return tmpNode;
			}
		#if USE_LFN
		}
		#endif
		
	loadCluster:
		//Load Next cluster?
		if( ((i+1) >> 4) % disk->bootsect.spc == 0 && ((i+1) & 0xF) == 0)
		{
			if( dirCluster == disk->rootOffset && disk->type != FAT32 )
				continue;
			dirCluster = FAT_int_GetFatValue(node->impl, dirCluster);
			vfs_seek(disk->fileHandle,
				(disk->firstDataSect+(dirCluster-2)*disk->bootsect.spc)*512,
				SEEK_SET);
		}
	}
	
	return NULL;
}

vfs_node *FAT_Mknod(vfs_node *node, char *name, int isdir)
{
	return NULL;
}

int FAT_Unlink(vfs_node *node)
{
	return 0;
}

int FAT_CloseFile(vfs_node *node)
{
	#if DEBUG
	LogF("FAT_CloseFile: (node=0x%x)\n", node);
	#endif
	if(node == NULL)	return 0;
	
	
	inode_uncacheNode(fat_disks[node->impl].inodeHandle, node->inode);
	#if USE_LFN
	if(inode_getCache(fat_disks[node->impl].inodeHandle, node->inode))
		return 1;
	if(node->flags & VFS_FFLAG_DIRECTORY)
		FAT_int_DelLFN(node);
	#endif
	#if DEBUG
	LogF("FAT_CloseFile: RETURN 1\n");
	#endif
	return 1;
}

void fat_install()
{	
	vfs_addfs(0x01, FAT_InitDevice);
	vfs_addfs(0x04, FAT_InitDevice);
	vfs_addfs(0x06, FAT_InitDevice);
	vfs_addfs(0x0B, FAT_InitDevice);
	vfs_addfs(0x0C, FAT_InitDevice);
	vfs_addfs(0x0E, FAT_InitDevice);
}
