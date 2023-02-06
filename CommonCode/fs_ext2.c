/*
Acess OS
Ext2Fs Driver Version 1
*/
#include <acess.h>
#include <vfs.h>
#include <fs_ext2_int.h>

//STRUCTURES
typedef struct {
	char	device[14];
	int		fileHandle;
	Uint8	groupCount;
	ext2_super_block	superBlock;
	ext2_group_desc		groups[32];
	vfs_node	*RootNode;
} ext2fs_disk;

//SEMI-GLOBALS
t_vfs_driver	driverStruct;
ext2fs_disk	ext2fs_disks[6];
 int	ext2fs_count = 0;

//PROTOTYPES
//Interface Functions
vfs_node	*ext2fs_initDisk(char *device);
 int	ext2fs_read(vfs_node *node, int offset, int length, void *buffer);
 int	ext2fs_write(vfs_node *node, int offset, int length, void *buffer);
vfs_node	*ext2fs_readdir(vfs_node *node, int dirPos);
vfs_node	*ext2fs_finddir(vfs_node *node, char *filename);
vfs_node	*ext2fs_mknod(vfs_node *node, char *name, int flags);
 int	ext2fs_int_getInode(int handle, unsigned int id, ext2_inode *inode);

//CODE
/**
 \fn vfs_node *ext2fs_initDevice(char *device)
 \brief Initializes a device to be read by by the driver
 \param device	String - Device to read from
 \return Root Node
*/
vfs_node *ext2fs_initDevice(char *device)
{
	 int	diskId = ext2fs_count;
	 int	a;
	Uint8	tmpdata[1024];
	ext2fs_disk		*disk;
	
	disk = &ext2fs_disks[diskId];	//Clean Up code by using a pointer
	
	disk->fileHandle = vfs_open(device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);		//Open Device
	if(disk->fileHandle == -1) {
		warning("ext2fs_initDisk - Unable to open '%s'\n", device);
		return NULL;
	}
	vfs_seek(disk->fileHandle, 1024, 0);	//Seek relative
	vfs_read(disk->fileHandle, 1024, &disk->superBlock);	//Read Superblock
	
	//Get Group count
	disk->groupCount = disk->superBlock.s_blocks_count / disk->superBlock.s_blocks_per_group;
	
	//Read Group Information
	vfs_read(disk->fileHandle, 1024, disk->groups);
	
	// Create Root Node
	disk->RootNode.name = "ext2";	disk->RootNode.nameLength = 4;
	disk->RootNode.inode = 2;	// Root Inode
	
	//Debug
	puts("EXT2 - Magic ");
	putHex(disk->superBlock.s_magic);
	puts(" Group Count ");
	putNum(disk->groupCount);
	
	return &disk->RootNode;
}

int ext2fs_read(vfs_node *node, int offset, int length, void *buffer) {
	return 0;
}

int ext2fs_write(vfs_node *node, int offset, int length, void *buffer) {
	return 0;
}

/**
 \fn vfs_node *ext2fs_readdir(vfs_node *node, int dirPos, char *filename)
 \brief Reads a directory entry
*/
vfs_node *ext2fs_readdir(vfs_node *node, int dirPos, char *filename)
{
	ext2_inode	inode;
	return 0;
}

/**
 \fn vfs_node *ext2fs_finddir(vfs_node *node, char *filename)
 \brief Gets information about a file
 \param node	vfs node - Parent Node
 \param filename	String - Name of file
 \return VFS Node of file
*/
vfs_node *ext2fs_finddir(vfs_node *node, char *filename)
{
	ext2_inode	inode;
	
	return -1;
}

int ext2fs_newfile(int handle, char *path, int isdir) {
	return 0;
}

void ext2fs_install()
{
	driverStruct.loadDisk = ext2fs_initDisk;
	driverStruct.read = ext2fs_read;
	driverStruct.write = ext2fs_write;
	driverStruct.readdir = ext2fs_readdir;
	driverStruct.fileinfo = ext2fs_fileinfo;
	driverStruct.newfile = ext2fs_newfile;
	driverStruct.loaded = 1;
	vfs_addfs(0x83, &driverStruct);
}

//==================================
//=       INTERNAL FUNCTIONS       =
//==================================

/**
 \internal
 \fn int ext2fs_int_getInode(vfs_node *node, ext2_inode *inode)
 \brief Gets the inode descriptor for a node
 \param node	
*/
int ext2fs_int_getInode(vfs_node *node, ext2_inode *inode)
{
	int	group, subId;
	
	//__asm__ ("div %0, %1"
	//		: "=r" (id), "=r" (ext2fs_disks[diskId].superBlock.s_inodes_per_group)
	//		: "a" (group), "d" (subId));
	
	group = id / ext2fs_disks[handle].superBlock.s_inodes_per_group;
	subId = id % ext2fs_disks[handle].superBlock.s_inodes_per_group;
	
	//Seek to Block - Absolute
	vfs_seek(ext2fs_disks[handle].fileHandle, ext2fs_disks[handle].superBlock.groups[group].bg_inode_table*1024, 1);
	//Seeek to inode - Relative
	vfs_seek(ext2fs_disks[handle].fileHandle, sizeof(ext2_inode)*subId, 0);
	vfs_read(ext2fs_disks[handle].fileHandle, sizeof(ext2_inode), inode);
	return 1;
}

