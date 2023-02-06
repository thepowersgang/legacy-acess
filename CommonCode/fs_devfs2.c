/*
Acess OS
Device Filesystem Version 2

Links device drivers to filesystem for
access by system. See bottom for export
list.
*/
//INCLUDES
#include <acess.h>
#include <vfs.h>
#include <fs_internal_int.h>
#include <fs_devfs2.h>

#define DEVFS_DEBUG	0
#define DEBUG (DEVFS_DEBUG | ACESS_DEBUG)

#define USE_LINKS	0

//SEMI-GLOBALS
devfs_driver	*devfs_drivers[MAX_DRIVERS] = {0};
#if USE_LINKS
vfs_node		devfs_links[MAX_LINKS];
#endif
vfs_node		devfs_node;
int				devfs_driverCount = 0;

//PROTOTYPES
vfs_node *devfs_readdir(vfs_node *dirNode, int pos);
vfs_node *devfs_finddir(vfs_node *dirNode, char *filename);

//CODE
/**
 \fn int dev_addDevice(devfs_driver *drv)
 \brief Adds a new device
 \param drv	Pointer to a devfs_driver structure
*/
int dev_addDevice(devfs_driver *drv)
{
	int	i = 0;
	
	for(;i<MAX_DRIVERS;i++)
	{
		if(devfs_drivers[i] == NULL)
		{
			devfs_drivers[i] = drv;
			break;
		}
	}
	if(i == MAX_DRIVERS)
	{
		warning("dev_addDevice - Out of driver space\n");
		return -1;
	}
	
	devfs_driverCount++;
	
	printf("[DEV ] Added Device '/Devices/%s'\n", drv->rootNode.name);
	
	return i;
}

/**
 \fn void dev_delDevice(int drv)
 \brief Removes a device
 \param drv	ID of driver to remove
*/
void dev_delDevice(int drv)
{
	devfs_drivers[drv] = NULL;
	devfs_driverCount--;
}

/**
 \fn void dev_addSymlink(int drv, char *path, char *name)
 \brief Adds a new symbolic link
 \param drv	Driver ID
 \param path	Path of destination
 \param name	Name of link
*/
int dev_addSymlink(int drv, char *path, char *name)
{
	#if USE_LINKS
	int i;
	for(i=0;i<MAX_LINKS;i++)
	{
		if(devfs_links[i].link == NULL)
		{
			int	start = 0;
			int	end = 0;
			vfs_node	*node = devfs_drivers[drv]->rootNode;
			while(path[start] != '\0')
			{
				end = 0;
				while(path[end] != '\0' && path[end] != '/')
					end++;
				if(path[end] == '\0')
					break;
				path[end] = '\0';
				node = node->finddir(node, path+start);
				if(!node)	return -1;
				path[end] = '/';
				start = end;
			}
			devfs_links[i].nameLength = strlen(name);
			devfs_links[i].name = (char*)malloc( devfs_links[i].nameLength + 1 );
			strcpy(devfs_links[i].name, name);
			devfs_links[i].link = node;
			devfs_links[i].mode = node->mode;
			devfs_links[i].flags = VFS_FFLAG_SYMLINK;
			return 1;
		}
	}
	#endif
	return -2;
}

/**
 \fn void dev_delSymlink(int id)
 \brief Removes a symbolic link
 \param id	Integer - ID of symbolic link to remove
 \todo Implement this
*/
void dev_delSymlink(int id)
{
	return;
}

//FILESYSTEM FUNCTIONS
/* Create DevFS driver instance
 */
vfs_node *dev_initDevice(char *device)
{
	int i;
	if(strcmp(device, "devfs") != 0)
		return NULL;
	
	for(i=0;i<MAX_DRIVERS;i++)
		devfs_drivers[i] = NULL;
	#if USE_LINKS
	for(i=0;i<MAX_LINKS;i++)
	{
		devfs_links[i].link = NULL;
		devfs_links[i].flags = VFS_FFLAG_SYMLINK;
		devfs_links[i].uid = 0;
		devfs_links[i].gid = 0;
	}
	#endif
	
	devfs_node.name = gsNullString;
	devfs_node.nameLength = 0;
	devfs_node.inode = 0;
	devfs_node.length = 0;
	devfs_node.flags = VFS_FFLAG_DIRECTORY;
	devfs_node.uid = 0;
	devfs_node.gid = 0;
	devfs_node.mode = 0111;	//--X--X--X - Directory
	devfs_node.ctime = devfs_node.mtime = devfs_node.atime = now();
	
	devfs_node.read = NULL;	devfs_node.write = NULL;
	devfs_node.unlink = NULL;
	devfs_node.readdir = devfs_readdir;
	devfs_node.finddir = devfs_finddir;
	devfs_node.mknod = NULL;	devfs_node.close = NULL;
	
	return &devfs_node;
}

/* Reads the directory entry at stated position
 */
vfs_node *devfs_readdir(vfs_node *dirNode, int pos)
{
	#if USE_LINKS
	if( !(0 <= pos && pos < MAX_DRIVERS+MAX_LINKS) )
		return NULL;
	#else
	if( !(0 <= pos && pos < MAX_DRIVERS) )
		return NULL;
	#endif
	
	#if USE_LINKS
	if(pos < MAX_DRIVERS)
	{
	#endif
		if(devfs_drivers[pos] == NULL)
			return &NULLNode;
		return &devfs_drivers[pos]->rootNode;
	
	#if USE_LINKS
	}
	#endif
	
	#if USE_LINKS
	if( devfs_links[pos-MAX_DRIVERS].link == NULL)
		return &NULLNode;
	return &devfs_links[pos-MAX_DRIVERS];
	#endif
}

/* Finds a directory entry by the name given
 */
vfs_node *devfs_finddir(vfs_node *dirNode, char *filename)
{
	int i;
	
	for(i=0;i<MAX_DRIVERS;i++)
	{
		if(devfs_drivers[i] == NULL)
			continue;
		if(strcmp(filename, devfs_drivers[i]->rootNode.name) == 0)
			return &devfs_drivers[i]->rootNode;
	}
	
	#if USE_LINKS
	for(i=0;i<MAX_LINKS;i++)
	{
		if( devfs_links[i-MAX_DRIVERS].link == NULL)
			continue;
		if( strcmp(filename, devfs_links[i-MAX_DRIVERS].name) == 0 )
			return &devfs_links[i-MAX_DRIVERS];
	}
	#endif
	
	return NULL;
}

/**
\fn void devfs_install()
**/
void devfs_install()
{
	vfs_addfs(DEVFS_ID, dev_initDevice);
}

/** 
\fn void devfs_ioctl(vfs_node *node, int id, void *data)
**/
int devfs_ioctl(vfs_node *node, int id, void *data)
{
	if(node == NULL || node->impl > MAX_DRIVERS)
		return -1;
	LogF("devfs_ioctl: (node->name='%s', id=%i, data=0x%x)\n", node->name, id, data);
	if(devfs_drivers[node->impl] == NULL)
		return -1;
	if(devfs_drivers[node->impl]->ioctl == NULL)
		return -1;
	return devfs_drivers[node->impl]->ioctl(node, id, data);
}

//==========
//= Function Export
//==========

Uint32	devfs_functions[] = {
	(Uint32)dev_addDevice,	//0
	(Uint32)dev_delDevice,	//1
	(Uint32)dev_addSymlink,	//2
	(Uint32)dev_delSymlink	//3
};
