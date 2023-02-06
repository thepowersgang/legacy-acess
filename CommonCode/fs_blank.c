/*
Acess OS
Blank VFS Driver Version 1
*/
// === INCLUDES ===
#include <acess.h>
#include <vfs.h>

// === SEMI-GLOBALS ===
t_vfs_driver	gDriverStruct;

// === PROTOTYPES ===

// === CODE ===
/**
 * \fn vfs_node *initDevice(char *device)
 * \brief Open a device as a filesystem
 * \param device	String - Filename of Device/File
 * \return Root node of filesystem
 */
vfs_node *initDevice(char *device)
{
	vfs_node	*node;
	//Allocate Node
	node = (vfs_node*) malloc(sizeof(vfs_node);
	//Set Name (By Convention the name of the root is "")
	node->name = NULL; node->nameLen = 0;
	//Set Callbacks
	// Root is a directory so Read and Write do not apply
	node->read = node->write = NULL;
	// Set Directory Callbacks
	node->readdir = readdir;
	node->finddir = finddir;
	node->mknod = mknod;
	node->close = closeDevice;
	return NULL;
}

int closeDevice(vfs_node *node)
{
}

int read(vfs_node *node, int offset, int length, void *buffer) {
	panic("Read Function not created yet.");
	return 1;
}
int write(vfs_node *node, int offset, int length, void *buffer) {
	return 0;
}
int mknod(vfs_node *node, char *path, int flags) {
	return 0;
}

/**
 \fn int readdir(vfs_node *node, int dirPos)
 \param node	Parent node
 \param dirPos	Directory position
 \return Child node or NULL on error / last entry
 \note Also returns &NULLNode on a blank, non-last entry
*/
vfs_node *readdir(vfs_node *node, int dirPos)
{
	return NULL;
}

/**
 \fn int finddir(vfs_node *node, char *name)
 \param node	Parent node
 \param name	Name of file to locate
 \return Child node or NULL
*/
vfs_node *finddir(vfs_node *node, char *name)
{
	return NULL;
}

void Install()
{
	driverStruct.loadDisk = initDisk;
	driverStruct.read = read;
	driverStruct.write = write;
	driverStruct.readdir = readdir;
	driverStruct.fileinfo = fileinfo;
	driverStruct.newfile = newfile;
	driverStruct.loaded = 1;
	
	vfs_addfs(0x0E, &driverStruct);
	vfs_addfs(0x0D, &driverStruct);
}
