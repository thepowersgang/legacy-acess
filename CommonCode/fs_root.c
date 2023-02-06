/*
Acess OS
Root Filesystem
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_internal_int.h>

#define	ROOTFS_DEBUG	0
#define	DEBUG	(ACESS_DEBUG|ROOTFS_DEBUG)

// === Semi-Globals ===
tRamFS_File	gRootFS_Root;

// === Prototypes ===
void	RootFS_Install();
vfs_node*	RootFS_InitDevice(char *device);
 int	RootFS_Read(vfs_node *node, int offset, int length, void *buffer);
 int	RootFS_Write(vfs_node *node, int offset, int length, void *buffer);
vfs_node*	RootFS_ReadDir(vfs_node *node, int dirPos);
vfs_node*	RootFS_FindDir(vfs_node *node, char *filename);
vfs_node*	RootFS_MkNod(vfs_node *node, char *filename, int isdir);

//Code
/* Install RootFS with the VFS handler
 */
void rootfs_install()
{	
	vfs_addfs(ROOTFS_ID, RootFS_InitDevice);
}

/* Initalise a device for use with RootFS
 * Essentially just confirms that RootFS is being legally
 * mounted and then returns the root node.
 *
 * char *device	- Device to mount (Must be 'root')
 *
 * Returns vfs_node*
 *  Pointer to root node
 */
vfs_node *RootFS_InitDevice(char *device)
{
	ramfs_file	*root;
	if(strcmp(device, "root") != 0)
		return NULL;
	
	gRootFS_Root.Parent = NULL;
	gRootFS_Root.NextSibling = NULL;
	gRootFS_Root.FirstChild = NULL;

	gRootFS_Root.Node.name = gsNullString;
	gRootFS_Root.Node.nameLength = 0;
	gRootFS_Root.Node.inode = &gRootFS_Root;
	gRootFS_Root.Node.flags = VFS_FFLAG_DIRECTORY;
	gRootFS_Root.Node.length = 0;
	gRootFS_Root.Node.uid = 0;
	gRootFS_Root.Node.gid = 0;
	gRootFS_Root.Node.mode = 0775;
	gRootFS_Root.Node.ctime = root->node.mtime = root->node.atime = now();
	
	gRootFS_Root.Node.read = NULL;
	gRootFS_Root.Node.write = NULL;
	gRootFS_Root.Node.unlink = NULL;
	gRootFS_Root.Node.readdir = RootFS_ReadDir;
	gRootFS_Root.Node.finddir = RootFS_FindDir;
	gRootFS_Root.Node.mknod = RootFS_MkNod;
	gRootFS_Root.Node.close = NULL;
	
	//Return
	return &gpRootFS_Root.node;
}

/* Read a node at a position in the directory
 *
 * vfs_node *node	- Directory node
 * int dirPos	- Node ID
 */
vfs_node *RootFS_ReadDir(vfs_node *node, int dirPos)
{
	 int	i;
	tRamFS_File	*dir;
	tRamFS_File	*file;
	
	if(!node || node->inode == 0)	return NULL;
	
	// Get File Struct
	dir = (void *)node->inode;
	
	// Do bounds checking
	if(dirPos >= dir->Length)	return NULL;
	
	// Traverse List
	file = dir->FirstChild;
	i = 0;
	while(file != NULL)
	{
		if(i == dirPos)		return &file->Node;
		file = file->NextSibling;
		i ++;
	}
	
	return NULL;
}

/* Find a file by name in directory
 *
 * vfs_node *node	- Directory Node
 * char *filename	- File to find
 */
vfs_node *RootFS_FindDir(vfs_node *node, char *filename)
{
	int i, count;
	int	*childList;
	tRamFS_File	*dir;
	tRamFS_File	*file;
	
	// Sanity Check
	if(!node || node->inode == 0)	return NULL;
	
	// Traverse List
	file = dir->FirstChild;
	while(file != NULL)
	{
		if(file->Node.name == NULL)
			continue;
		if(strcmp(filename, file->Node.name) == 0)
			return &file->Node;
	}
	
	return NULL;
}

/* Make a node (file)
 *
 * vfs_node *node	- Parent Node
 * char *filename	- Name of new node
 * int flags	- Node Flags
 */
vfs_node *RootFS_MkNod(vfs_node *node, char *filename, int flags)
{
	int	inode, i;
	vfs_node	*newNode;
	tRamFS_File	*parent;
	tRamFS_File	*file;
	
	#if DEBUG
		LogF("RootFS_MkNod: (node=*0x%x, filename='%s', isdir=%i)\n", (Uint)node, filename, isdir);
	#endif
	
	if(node == NULL || node->inode == 0)	return NULL
	
	// Create new file
	file = malloc(sizeof(tRamFS_File));
	// Error check
	if(file == NULL) {
		warning("RootFS_MkNod - Unable to allocate new file\n");
		return NULL;
	}
	
	// Get Parent
	parent = (tRamFS_File*) node->inode;
	
	// Create Child
	file.Node.nameLength = strlen(filename);
	file.Node.name = (char*)malloc(newNode->nameLength+1);
	memcpy(file.Node.name, filename, file.Node.nameLength+1);
	file.Node.inode = inode;
	file.Node.flags = flags;
	file.Node.length = 0;
	file.Node.uid = 0;
	file.Node.gid = 0;
	file.Node.mode = 0775;
	file.Node.ctime = file.Node.mtime = file.Node.atime = now();
	
	//Set Functions
	if(flags & VFS_FFLAG_DIRECTORY) {
		file.Node.readdir = RootFS_ReadDir;
		file.Node.finddir = RootFS_FindDir;
		file.Node.mknod = RootFS_MkNod;
		file.Node.read = NULL;
		file.Node.write = NULL;
	} else {
		file.Node.readdir = NULL;
		file.Node.finddir = NULL;
		file.Node.mknod = NULL;
		file.Node.read = RootFS_Read;
		file.Node.write = RootFS_Write;
	}
	file.Node.close = NULL;
	file.Node.unlink = NULL;
	
	return newNode;
}

/**
 \fn int RootFS_Read(vfs_node *node, int offset, int length, void *buffer)
*/
int RootFS_Read(vfs_node *node, int offset, int length, void *buffer)
{
	if(node == NULL)	return 0;
	return 0;
}

/**
 \fn int RootFS_Write(vfs_node *node, int offset, int length, void *buffer)
*/
int RootFS_Write(vfs_node *node, int offset, int length, void *buffer)
{
	if(node == NULL)	return 0;
	return 0;
}
