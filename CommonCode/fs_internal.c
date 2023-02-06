/*
Acess OS
Internal Filesystems Version 1
Includes RootFS
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_internal_int.h>

#define	ROOTFS_DEBUG	0
#define	DEBUG	(ACESS_DEBUG|ROOTFS_DEBUG)

//SEMI-GLOBALS
ramfs_file	rootfs2Files[MAX_ROOT];	//Files
tRamFS_File	gpRootFS_Root;

//PROTOTYPES
void rootfs_install();
vfs_node	*rootfs_initDevice(char *device);
int	rootfs_read(vfs_node *node, int offset, int length, void *buffer);
int	rootfs_write(vfs_node *node, int offset, int length, void *buffer);
vfs_node*	rootfs_readdir(vfs_node *node, int dirPos);
vfs_node*	rootfs_finddir(vfs_node *node, char *filename);
vfs_node*	rootfs_mknod(vfs_node *node, char *filename, int isdir);

//Code
/* Install RootFS with the VFS handler
 */
void rootfs_install()
{	
	vfs_addfs(ROOTFS_ID, rootfs_initDevice);
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
vfs_node	*rootfs_initDevice(char *device)
{
	ramfs_file	*root;
	if(strcmp(device, "root") != 0)
		return NULL;
	
	//Set initial file
	root = &rootfs2Files[0];
	root->alloc = 1;	//Mark as Allocated
	root->childCount = 0;	//Set Child Count
	root->childMax = CHILD_ARR_STEP;	//Set Child Maximum (size of children array)
	root->children = (int*)malloc(CHILD_ARR_STEP*sizeof(int));	//Set Children array to NULL
	memset(root->children, 0, CHILD_ARR_STEP*sizeof(int));
	
	//Set Attributes
	root->node.name = gsNullString;
	root->node.nameLength = 0;
	root->node.inode = 0;
	root->node.flags = VFS_FFLAG_DIRECTORY;
	root->node.length = 0;
	root->node.uid = 0;
	root->node.gid = 0;
	root->node.mode = 0775;
	root->node.ctime = root->node.mtime = root->node.atime = now();
	
	//Set Functions
	root->node.read = NULL;
	root->node.write = NULL;
	root->node.unlink = NULL;
	root->node.readdir = rootfs_readdir;
	root->node.finddir = rootfs_finddir;
	root->node.mknod = rootfs_mknod;
	root->node.close = NULL;
	
	//Return
	return &root->node;
}

/* Read a node at a position in the directory
 *
 * vfs_node *node	- Directory node
 * int dirPos	- Node ID
 */
vfs_node*	rootfs_readdir(vfs_node *node, int dirPos)
{
	if(!node || node->inode > MAX_ROOT)
		return NULL;
	
	if(dirPos >= rootfs2Files[ node->inode ].childMax)
		return NULL;
	
	if( rootfs2Files[ node->inode ].children[ dirPos ] == 0)
		return &NULLNode;
	
	return	&rootfs2Files[ rootfs2Files[ node->inode ].children[dirPos] ].node;
}

/* Find a file by name in directory
 *
 * vfs_node *node	- Directory Node
 * char *filename	- File to find
 */
vfs_node*	rootfs_finddir(vfs_node *node, char *filename)
{
	int i, count;
	int	*childList;
	
	count = rootfs2Files[ node->inode ].childMax;
	childList = rootfs2Files[ node->inode ].children;
	
	for(i=0;i<count;i++)
	{
		if(rootfs2Files[ childList[i] ].node.name == NULL)
			continue;
		if(strcmp(filename, rootfs2Files[ childList[i] ].node.name) == 0)
			return &rootfs2Files[ childList[i] ].node;
	}
	
	return NULL;
}

/* Make a node (file)
 *
 * vfs_node *node	- Parent Node
 * char *filename	- Name of new node
 * int flags	- Node Flags
 */
vfs_node*	rootfs_mknod(vfs_node *node, char *filename, int flags)
{
	int	inode, i;
	vfs_node	*newNode;
	
	#if DEBUG
		LogF("rootfs_mknod: (node=*0x%x, filename='%s', isdir=%i)\n", (Uint)node, filename, isdir);
	#endif
	
	if(flags & VFS_FFLAG_DIRECTORY)	//At the moment directories only
	{
		warning("rootfs_mknod - Directories Only Please\n");
		return NULL;
	}
	
	//Find a free inode
	for(inode=0;inode<MAX_ROOT;inode++)
	{
		if(rootfs2Files[inode].alloc == 0)
			break;
	}
	
	if(inode == MAX_ROOT)
	{
		warning("rootfs_mknod - RootFS Inode Pool Exhausted\n");
		return NULL;
	}
	
	#if DEBUG
		LogF(" rootfs_mknod: node->inode=%i,inode=%i, childCount=%i\n", node->inode, inode, rootfs2Files[ node->inode ].childCount);
	#endif
	
	for(i = 0; i < rootfs2Files[ node->inode ].childMax; i++)
	{
		if(rootfs2Files[ node->inode ].children[ i ] == 0)
			break;
	}
	if(i > rootfs2Files[ node->inode ].childCount)
		rootfs2Files[ node->inode ].childCount ++;
	
	// Check Space
	if(i == rootfs2Files[ node->inode ].childMax && i == rootfs2Files[ node->inode ].childMax)
	{
		void *tmp;
		tmp = realloc( (i + CHILD_ARR_STEP)*sizeof(int), rootfs2Files[ node->inode ].children );
		if(!tmp)
		{
			warning("Unable to allocate space for file `%s' in RootFS\n", filename);
			return NULL;
		}
		rootfs2Files[ node->inode ].children = tmp;
		memset(&rootfs2Files[ node->inode ].children[i], 0, CHILD_ARR_STEP*sizeof(int));
		rootfs2Files[ node->inode ].childMax += CHILD_ARR_STEP;
	}
	
	rootfs2Files[ node->inode ].children[ i ] = inode;
	// Increment Parent Child Count
	rootfs2Files[ node->inode ].childCount++;
	rootfs2Files[ node->inode ].node.length = rootfs2Files[ node->inode ].childCount;
	
	// Set file properties
	rootfs2Files[inode].alloc = 1;
	rootfs2Files[inode].childCount = 0;
	rootfs2Files[inode].childMax = 0;
	newNode = &rootfs2Files[inode].node;
	newNode->nameLength = strlen(filename);
	newNode->name = (char*)malloc(newNode->nameLength+1);
	memcpy(newNode->name, filename, newNode->nameLength+1);
	newNode->inode = inode;
	newNode->flags = VFS_FFLAG_DIRECTORY;
	newNode->length = 0;
	newNode->uid = 0;
	newNode->gid = 0;
	newNode->mode = 0775;
	newNode->ctime = newNode->mtime = newNode->atime = now();
	
	//Set Functions
	newNode->read = NULL;
	newNode->write = NULL;
	newNode->readdir = rootfs_readdir;
	newNode->finddir = rootfs_finddir;
	newNode->mknod = rootfs_mknod;
	newNode->close = NULL;
	
	return newNode;
}
