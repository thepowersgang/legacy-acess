/*
Acess OS
Initrd Driver Version 1
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_rom_int.h>

#define ROM_DEBUG	1
#define DEBUG	(ROM_DEBUG | ACESS_DEBUG)

//SEMI-GLOBALS
drv_rom_volinfo	rom_gDisks[8];
int		rom_partCount = 0;

//PROTOTYPES
vfs_node *rom_readdir(vfs_node *node,  int dirPos);
vfs_node *rom_finddir(vfs_node *dirNode, char *file);

//CODE
vfs_node *rom_initDevice(char *device)
{
	vfs_node	*node;
	rom_header	*hdr = &rom_gDisks[rom_partCount].header;
	
	#if DEBUG
	printf("rom_initDevice: (device='%s')\n", device);
	#endif
	
	//Get Address
	rom_gDisks[rom_partCount].start  = (device[0]-'A')<<28;
	rom_gDisks[rom_partCount].start |= (device[1]-'A')<<24;
	rom_gDisks[rom_partCount].start |= (device[2]-'A')<<20;
	rom_gDisks[rom_partCount].start |= (device[3]-'A')<<16;
	rom_gDisks[rom_partCount].start |= (device[4]-'A')<<12;
	rom_gDisks[rom_partCount].start |= (device[5]-'A')<<8;
	rom_gDisks[rom_partCount].start |= (device[6]-'A')<<4;
	rom_gDisks[rom_partCount].start |= (device[7]-'A')<<0;
	
	//Mark Data Start
	rom_gDisks[rom_partCount].data = (Uint32*) rom_gDisks[rom_partCount].start;
	
	//Copy to header
	memcpy(hdr, rom_gDisks[rom_partCount].data, sizeof(rom_header));
	//Set dir structure pointer
	rom_gDisks[rom_partCount].dir = (rom_dirent*) (rom_gDisks[rom_partCount].start + hdr->rootOff);
	
	//Check header id (Magic) - Should be 'IRF\0'
	if(hdr->id[0] != 'I' || hdr->id[1] != 'R' || hdr->id[2] != 'F' || hdr->id[3] != '\0') {
		printf("There is not a valid init rom file at 0x%x\n", rom_gDisks[rom_partCount].start);
		printf(" Code: %x%x%x%x", hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
		printf("(%c%c%c%c)\n", hdr->id[0], hdr->id[1], hdr->id[2], hdr->id[3]);
		return NULL;
	}
	
	//Allocate and populate root node
	#if DEBUG
	printf(" rom_initDevice: Allocating root VFS Node\n");
	#endif
	node = (vfs_node*)malloc(sizeof(vfs_node));
	#if DEBUG
	printf(" rom_initDevice: Address: %x\n", (Uint32)node);
	#endif
	node->name = NULL;	node->nameLength = 0;	node->inode = -1;	node->impl = rom_partCount;
	node->uid = 0;	node->gid = 0;	node->mode = 0775;	//root,root RWXRWXR-X
	node->flags = VFS_FFLAG_DIRECTORY | VFS_FFLAG_READONLY;	//Readonly Directory
	node->ctime = node->mtime = node->atime = now();
	
	node->readdir = rom_readdir;
	node->finddir = rom_finddir;
	node->read = NULL;
	node->write = NULL;
	node->mknod = NULL;
	node->close = NULL;
	
	rom_gDisks[rom_partCount].node = node;
	rom_gDisks[rom_partCount].dirNodes = (vfs_node*)malloc(sizeof(vfs_node)*hdr->count);
	memset(rom_gDisks[rom_partCount].dirNodes, sizeof(vfs_node)*hdr->count, '\0');
	
	rom_partCount++;
	
	return node;
}

/**
 \fn int rom_read(vfs_node *node, int offset, int length, void *buffer)
 \param node		Node to read from
 \param offset	Start address in file
 \param length	Length of data
 \param buffer	Output buffer
 \return boolean - Sucess
**/
int rom_read(vfs_node *node, int offset, int length, void *buffer) {
	Uint32	*dat;
	#if DEBUG
	printf("rom_read: (offset=%i, length=%i, buffer=0x%x)\n", offset, length, (Uint32)buffer);
	#endif
	node->atime = now();
	dat = (Uint32*)(rom_gDisks[node->impl].start + rom_gDisks[node->impl].dir[node->inode].offset + offset);
	#if DEBUG
	printf(" rom_read: Updated access time (%i)\n", node->atime);
	printf(" rom_read: dat=0x%x\n", (Uint32)dat);
	#endif
	memcpy(buffer, dat, length);
	return 1;
}

/**
 \fn vfs_node *rom_readdir(vfs_node *node,  int dirPos)
 \param node		Node structure pointer
 \param dirPos	Directory position
 \return Node Structure of file
**/
vfs_node *rom_readdir(vfs_node *node, int dirPos)
{
	vfs_node	*retNode;
	
	#if DEBUG
	printf("rom_readdir: (node->inode=%i, dirPos=%i)\n", node->inode, dirPos);
	#endif
	
	if(dirPos >= rom_gDisks[node->impl].header.count) {
		return NULL;
	}
	
	//Get Node
	retNode = &rom_gDisks[node->impl].dirNodes[dirPos];
	//Check if it has been populated
	if(retNode->nameLength > 0)
		return retNode;
	
	#if DEBUG
	printf(" rom_readdir: Initialising Node\n");
	#endif
	
	//Else Populate it
	retNode->name = rom_gDisks[node->impl].dir[dirPos].name;
	retNode->nameLength = rom_gDisks[node->impl].dir[dirPos].namelen;
	retNode->inode = dirPos;	//File ID
	retNode->length = rom_gDisks[node->impl].dir[dirPos].size;
	retNode->impl = node->impl;	//Disk ID
	
	retNode->uid = 0;	retNode->gid = 0;	retNode->mode = 0775;
	retNode->flags = VFS_FFLAG_READONLY;
	retNode->ctime = retNode->mtime = retNode->atime = rom_gDisks[node->impl].node->ctime;
	
	retNode->read = rom_read;
	retNode->write = NULL;
	retNode->readdir = NULL;
	retNode->finddir = NULL;
	retNode->mknod = NULL;
	
	#if DEBUG
	printf("  rom_readdir: retNode={name='%s',length=%i}\n", retNode->name, retNode->length);
	#endif
	
	return retNode;
}

/* Find a entry by name
 */
vfs_node *rom_finddir(vfs_node *dirNode, char *file)
{
	int 	i=0, namelen;
	int		handle = dirNode->impl;
	
	namelen = strlen(file);
	
	//Scan Directory Structure
	for(i=0; i<rom_gDisks[handle].header.count; i++) {
		#if DEBUG
		printf("%s == %s\n", file, rom_gDisks[handle].dir[i].name);
		#endif
		if(namelen != rom_gDisks[handle].dir[i].namelen)
			continue;
		if(strcmp(file, rom_gDisks[handle].dir[i].name) != 0) {
			continue;
		}
		
		break;
	}
	//If no file was found
	if(i == rom_gDisks[handle].header.count) {
		return NULL;
	}
	
	#if DEBUG
	printf(" rom_finddir: File #%i => '%s'\n", i, file);
	#endif
	//Pass on to rom_readdir
	return rom_readdir(dirNode, i);
}

void rom_install()
{	
	vfs_addfs(INITROMFS_ID, rom_initDevice);
}
