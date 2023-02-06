/*
Acess OS
Virtual File System Version 1.5
- New File Descriptor Layout
*/
#include <acess.h>
#include <vfs.h>
#include <vfs_int.h>

#define VFS_DEBUG	0
#define DEBUG	(VFS_DEBUG | ACESS_DEBUG)
#define	DEBUG_WARN	1

//CONSTANTS
#define	MAX_MOUNTS	16
#define FH_K_FLAG	0x8000

//GLOBAL VARIABLES
t_fs_initDev	vfsDrivers[512];
t_vfs_mount		mounts[MAX_MOUNTS];
tFileHandle		*gpFirstHandle = NULL;
tFileHandle		*gpKernelHandles = NULL;
vfs_node		NULLNode;
unsigned int	mountCount = 0;
unsigned int	giVfsLastFile = 0;
unsigned int	giVfsLastKFile = 0;

//IMPORTS
extern void	devfs_install();	// Device File System - Integral to system
extern int	devfs_ioctl(vfs_node *node, int id, void *data);

//extern void ext2fs_install();	// EXT2FS Driver
extern void fat_install();		// FAT Driver
extern void rootfs_install();	// Root File System
extern void rom_install();		// Basic ROM Ramdisk Driver

// ==== PROTOTYPES ====
int vfs_checkPerms(vfs_node *node, int req);
tFileHandle *vfs_getFileHandle(int id);

//CODE
/*!	\fn void vfs_install(void)
 *	\brief Install and setup Virtual Filesystem layer
 *
 *	vfs_install is the 'entry point' of the VFS system, it loads
 *	all the file systems and then initalizes the core filesystems,
 *	RootFS and DevFS.
 */
void vfs_install(void)
{
	int i;
	
	gpFirstHandle = (void*)0xE4000000;
	gpKernelHandles = (void *)0xC4000000;
	
	//Initalise Driver List
	for(i=0;i<512;i++)		vfsDrivers[i] = NULL;
	
	// Load Core FS Drivers
	rootfs_install();
	devfs_install();
	
	// Create Filesystem
	// TODO: Allow dynamic setup in `config.c`
	vfs_mount("root", "/", ROOTFS_ID);
	mounts[0].mountPointLen = 0;	//Hack (Used to ignore the first '/')
	vfs_mknod("/Devices", 1);
	vfs_mknod("/Mount", 1);
	vfs_mount("devfs", "/Devices", DEVFS_ID);
	
	// Initialise NULLNode
	NULLNode.name = NULL;	NULLNode.nameLength = 0;
	NULLNode.impl = -1;	NULLNode.inode = -1;
	NULLNode.mode = 0;	NULLNode.uid = 0;	NULLNode.gid = 0;
	NULLNode.read = NULL;	NULLNode.write = NULL;	NULLNode.close = NULL;
	NULLNode.readdir = NULL;	NULLNode.finddir = NULL;	NULLNode.unlink = NULL;
}

/*!	\fn int vfs_addfs(Uint16 ident, t_fs_initDev initDev)
 *	\brief Add a filesystem to be handled
 *	\param ident Filesystem identifier - read from MBR
 *	\param initDev Function pointer to filesystem initializer
 *
 *	This function is called by filesystem drivers to make the vfs layer aware
 *	of the filesystems it can read and the addresses of its functions.
 */
int vfs_addfs(Uint16 ident, t_fs_initDev initDev)
{
	if(vfsDrivers[ident] != NULL) {
		return 0;
	}
	LogF("[VFS ] Filesystem 0x%x Added\n", ident);
	vfsDrivers[ident] = initDev;
	return 1;
}

/*!	\fn int vfs_open(char *path, int flags)
 *	\brief Open a file for reading/writing
 *	\param path	String containing path to open
 *	\param flags	Flags
 */
int vfs_open(char *path, int flags) 
{
	 int	mnt, tmpCheck, i;
	 int	longestMnt = -1;
	 int	intHandle = 0;
	char	*tmpName;
	 int	tmpNameLen = 0, sectionCount = 0;
	 int	sectionOffsets[255];
	vfs_node	*fileNode;
	tFileHandle	*handle;
	
	if(path == NULL)
		return -1;
	
	#if DEBUG
		LogF("vfs_open: (path='%s', flags=%i)\n", path, flags);
	#endif
	
	// --- Get Handle
	tFileHandle	*hs;
	 int	last;
	 
	// Get Pointers
	if(flags & VFS_OPENFLAG_USER) {
		hs = gpFirstHandle;
		last = giVfsLastFile;
	} else {
		hs = gpKernelHandles;
		last = giVfsLastKFile;
	}
	
	// Check for free entry
	for( intHandle = 0; intHandle < last; intHandle++ ) {
		if( hs[ intHandle ].node == NULL)
			break;
	}
	
	// If list is full
	if( intHandle == last )
	{
		Uint	addr = ( (Uint)(hs + last) + 0xFFF) & 0xFFFFF000;
		intHandle = last;
		last = (addr+0x1000-(Uint)hs) / sizeof( *hs );
		if( (addr-(Uint)hs) > 0x80000000 && flags & VFS_OPENFLAG_USER) {
			warning("vfs_open - Max file limit of %i files reached in process #%i\n", last, proc_pid);
			return -1;
		}
		if( (addr-(Uint)hs) > 0x40000000 && !(flags & VFS_OPENFLAG_USER) ) {
			warning("vfs_open - Max file limit of %i files reached in kernel\n", last);
			return -1;
		}
		mm_alloc( addr );
		memsetda( (void*)addr, 0, 1024 );
		if(flags & VFS_OPENFLAG_USER)
			giVfsLastFile = last;
		else
			giVfsLastKFile = last;
	}
	
	// Get Handle Structure
	handle = &hs[ intHandle ];
	
	// Add Kernel Flag
	if( !(flags & VFS_OPENFLAG_USER) )
		intHandle |= FH_K_FLAG;
	// --- END
	
	// ---- Check Mountpoints ----
	for(mnt=0;mnt<mountCount;mnt++)
	{
		tmpCheck = strcmp(path, mounts[mnt].mountPoint);
		// Root of Filesystem
		if(tmpCheck == '\0') {
			if(mounts[mnt].rootNode == NULL)
			{
				warning("vfs_open - %s is mounted incorrectly\n", mounts[mnt].mountPoint);
				return -1;
			}
			handle->mount = mnt;
			handle->pos = 0;
			handle->node = mounts[mnt].rootNode;
			handle->mode = flags & VFS_OPENFLAG_MASK;
			
			// --- Check Permissions ---
			if(flags & VFS_OPENFLAG_USER)
			{
				i = 0;
				if(flags & VFS_OPENFLAG_READ)	i |= 4;
				if(flags & VFS_OPENFLAG_WRITE)	i |= 2;
				if(flags & VFS_OPENFLAG_EXEC)	i |= 1;
				if(vfs_checkPerms(mounts[mnt].rootNode, i) == 0)
					return -1;
			}
			#if DEBUG
				LogF("vfs_open: `%s' opened as 0x%x\n", path, intHandle);
			#endif
			return intHandle;
		}
		
		// Below the root - Needs to be checked so that a mount on a mount is valid
		if(tmpCheck == '/') {
			if(mounts[mnt].mountPointLen >  mounts[longestMnt].mountPointLen)
				longestMnt = mnt;
		}
	}
	
	// ---- If no mount point was found ----
	if(longestMnt == -1)
		mnt = 0;	// Use root
	else
		mnt = longestMnt;	// Use the most satisfactory mount
	
	fileNode = mounts[mnt].rootNode;
	if(fileNode == NULL)
	{
		warning("vfs_open - %s is mounted incorrectly\n", mounts[mnt].mountPoint);
		return -1;
	}
	
	// ---- Check Permissions on mount point ----
	if(flags & VFS_OPENFLAG_USER && vfs_checkPerms(fileNode, 1) == 0)	// Required --X
		return -1;
	
	// ---- Break the path down ----
	tmpNameLen = strlen(path) - mounts[mnt].mountPointLen - 1;
	tmpName = (char *)malloc(tmpNameLen+1);
	memcpy(tmpName, (char *)(path+mounts[mnt].mountPointLen+1), tmpNameLen+1);
	#if DEBUG >= 2
		LogF(" vfs_open: tmpName=%s\n", tmpName);
	#endif
	
	sectionCount = 1;
	sectionOffsets[0] = 0;
	for(i=0;i<tmpNameLen;i++)
	{
		if(tmpName[i] == '/')
		{
			sectionOffsets[sectionCount] = i+1;
			sectionCount++;
			tmpName[i] = '\0';
		}
	}
	
	#if DEBUG >= 2
		LogF(" vfs_open: sectionCount=%i\n", sectionCount);
	#endif
	
	// ---- Parse Path ----
	for(i=0;i<sectionCount;i++)
	{
		// ---- Check Permissions ----
		if(flags & VFS_OPENFLAG_USER && vfs_checkPerms(fileNode, 1) == 0)	// Required --X
		{
			#if DEBUG
				warning("vfs_open: Unable to traverse parent of `%s' in path `%s'\n", tmpName, path);
			#endif
			free(tmpName);
			return -1;
		}
		// --- Check if the Directory is traversable
		if(fileNode->finddir == NULL)
		{
			free(tmpName);
			return -1;	//Or maybe return fileNode?
		}
		// --- Find Child
		fileNode = fileNode->finddir(fileNode, (char*)(tmpName+sectionOffsets[i]));
		if(fileNode == NULL)
		{
			#if DEBUG
				warning("vfs_open: Unable to find file `%s' of `%s'\n", tmpName, path);
			#endif
			free(tmpName);
			return -1;
		}
		// --- Handle Symbolic Links
		if(fileNode->flags & VFS_FFLAG_SYMLINK && fileNode->link != NULL)
			fileNode = fileNode->link;
		// --- Repair Buffer
		if(i > 0)
			tmpName[sectionOffsets[i]-1] = '/';
	}
	//Free Tempory Buffer
	free(tmpName);
	
	// --- Check Permissions ---
	if(flags & VFS_OPENFLAG_USER)
	{
		i = 0;
		if(flags & VFS_OPENFLAG_READ)	i |= 4;
		if(flags & VFS_OPENFLAG_WRITE)	i |= 2;
		if(flags & VFS_OPENFLAG_EXEC)	i |= 1;
		if(vfs_checkPerms(fileNode, i) == 0)
			return -1;
	}
	
	//Set handle info
	handle->mount = mnt;
	handle->mode = flags & VFS_OPENFLAG_MASK;
	handle->node = fileNode;
	handle->pos = 0;
	
	#if DEBUG
		LogF("vfs_open: `%s' opened as 0x%x\n", path, intHandle);
	#endif
	
	return intHandle;
}

/*!	\fn int vfs_opendir(char *path)
 *	\brief Open a directory for reading
 *	\param path String containing path to open as dir
 *
 *	This is just a dummy function that passes control to
 *	vfs_open as they are functionally identical
 */
int vfs_opendir(char *path) 
{
	return vfs_open(path, VFS_OPENFLAG_EXEC);
}

/*!	\fn int vfs_readdir(int fh, char *name)
 *	\brief Reads the next file from a directory
 *	\param fh	Handle to directory returned by vfs_opendir
 *	\param name	Pointer to string where name of file will be put
 *
 *	This function is calls the relevant function of the filestsyem driver
 *	to read the directory
 */
int vfs_readdir(int fh, char *name)
{
	vfs_node	*dirNode;
	vfs_node	*childNode;
	tFileHandle	*handle;
	
	#if DEBUG >= 2
		LogF("vfs_readdir: (fh=0x%x, name=*0x%x)\n", fh, name);
	#endif
	
	handle = vfs_getFileHandle( fh );
	
	// Check validity of handle
	if(handle == NULL)
		return 0;
	
	// Ignore Null Destination
	if(name == NULL)
		return -1;
	
	// Get Node
	dirNode = handle->node;
	
	// Check if the node is a directory
	if(!(dirNode->flags & VFS_FFLAG_DIRECTORY) || dirNode->readdir == NULL)
		return -2;
	if( !(handle->mode & VFS_OPENFLAG_EXEC) )
		return -3;
	
	// Read entry skipping empty/ignored entries
	for(;;) {
		childNode = dirNode->readdir(dirNode, handle->pos);
		if(childNode == NULL)	break;
		if((int)childNode < 0) {
			handle->pos++;
			if(childNode != &NULLNode)	break;
			
		}
		else
			handle->pos += (int)childNode;
	}
	
	//End of list
	if(childNode == NULL)
		return 0;
	
	if(childNode->name == NULL)
		return 0;
	
	//Copy to name
	memcpy(name, childNode->name, childNode->nameLength+1);
	
	// Close Node
	if(childNode->close)
		childNode->close(childNode);
	
	return 1;
}

/*!	\fn int vfs_read(int fh, int length, void *buffer)
 *	\brief Read data from a file
 *	\param fh	Handle returned by vfs_open
 *	\param length	Byte ammount of data to read
 *	\param buffer	Place to put data read
 */
int vfs_read(int fh, int length, void *buffer)
{
	vfs_node	*node;
	 int	ret;
	tFileHandle	*handle = vfs_getFileHandle( fh );
	
	if(handle == NULL)
		return -1;
	
	#if DEBUG >= 2
		LogF("vfs_read: (fh=0x%x,length=%i,buffer=*0x%x)\n", fh, length, (Uint)buffer);
		LogF(" vfs_read: mount = %i\n", handle->mount);
	#endif
	
	if(!(handle->mode & VFS_OPENFLAG_READ))	// Check Mode
	{
		warning("vfs_read - Non-Allowed Read fh=0x%x, (Perms: 0%o)\n", fh, handle->mode);
		return 0;
	}
	
	if(length <= 0)
	{
		#if DEBUG >= 3
			LogF(" vfs_read: 0 bytes to read, quick returning\n");
		#endif
		return 0;
	}
	
	node = handle->node;
	#if DEBUG >= 3
		LogF(" vfs_read: node = (vfs_node*)(0x%x)\n", (Uint)node);
	#endif
	
	if(node->length > 0 && handle->pos+length > node->length) {
		//warning("vfs_read - Read past end of file, truncated. (0x%x+0x%x > 0x%x)\n", handle->pos,length,node->length);
		if(handle->pos > node->length)
			return 0;
		length = node->length - handle->pos;
	}
	
	#if DEBUG >= 3
		LogF(" vfs_read: Read from %i to %i\n", handle->pos, handle->pos+length);
	#endif
	
	if(node->read == NULL)
	{
		#if DEBUG_WARN
			warning(" vfs_read - No Read method for node (fp=0x%x,node->name='%s')\n", handle, node->name);
		#endif
		return -1;
	}
	 
	ret = node->read(node, handle->pos, length, buffer);
	handle->pos += length;
	//return ret;
	return length;	// Return number of bytes read
}

/*!	\fn int vfs_write(int fh, int length, void *buffer)
 *	\brief Read data from a file
 *	\param fh	Handle returned by vfs_open
 *	\param length	Byte ammount of data to write
 *	\param buffer	Place to put data write
 */
int vfs_write(int fh, int length, void *buffer)
{
	vfs_node	*node;
	int ret;
	tFileHandle	*handle = vfs_getFileHandle( fh );
	
	#if DEBUG >= 2
		LogF("vfs_write: (fh=0x%x, length=%i)\n", fh, length);
	#endif
	
	if(handle == NULL)
		return -1;
	
	if(length == 0)
	{
		#if DEBUG >= 3
			LogF(" vfs_write: 0 bytes to write, quick returning\n");
		#endif
		return 0;
	}
	
	if( !(handle->mode & VFS_OPENFLAG_WRITE))	// Check Mode
		return 0;
	
	node = handle->node;
	
	if(node->write == NULL){
		#if DEBUG_WARN
		warning("vfs_write - No Write Function. fh=0x%x, name='%s'\n", fh, node->name);
		#endif
		return -1;
	}
	ret = node->write(node, handle->pos, length, buffer);
	handle->pos += length;
	return ret;
}

/*!	\fn int vfs_seek(int fh, int distance, int flag)
 *	\brief Alter file position
 *	\param fh	Handle to seek
 *	\param distance	Ammount to seek by
 *	\param flag	How to seek (0: Relative, 1:Absolute, -1:End)
 */
void vfs_seek(int fh, int distance, int flag)
{
	tFileHandle	*h = vfs_getFileHandle( fh );
	if(h == NULL)	return;
	switch(flag)
	{
	case 0:
		h->pos += distance;
		return;
	case 1:
		h->pos = distance;
		return;
	case -1:
		h->pos = h->node->length - distance;
		return;
	}
}

/*!	\fn int vfs_tell(int fh)
 *	\brief Returns the file position
 *	\param fh	Handle
 */
int vfs_tell(int fh)
{
	tFileHandle	*h = vfs_getFileHandle( fh );
	if(h == NULL)	return -1;
	return h->pos;
}

/*!	\fn int vfs_close(int fh)
 *	\brief Closes a file
 *	\param fh	Handle to close
 */
void vfs_close(int fh)
{
	tFileHandle	*h = vfs_getFileHandle( fh );
	#if DEBUG
		LogF("vfs_close: (fileHandle=0x%x)\n", fh);
	#endif
	
	if(h == NULL)	return;
	
	if( h->node && h->node->close )
		h->node->close(h->node);
		
	h->mount = -1;
	h->node = NULL;
}

/*!	\fn int vfs_stat(int fh, t_fstat *info)
 *	\brief Returns File information
 *	\param fh	Integer - File Pointer
 *	\param info	Pointer - Pointer to t_fstat structure
 */
int vfs_stat(int fh, t_fstat *info)
{
	vfs_node	*node;
	tFileHandle	*h = vfs_getFileHandle( fh );
	
	if( h == NULL )	return 0;
	
	node = h->node;
	
	info->st_dev = h->mount;
	info->st_ino = node->inode;
	info->st_mode = node->mode;
	if(node->flags & VFS_FFLAG_DIRECTORY)
	{
		info->st_mode |= S_IFDIR;
	}
	info->st_nlink = 1;
	info->st_uid = node->uid;
	info->st_gid = node->gid;
	info->st_rdev = 0;
	info->st_size = node->length;
	info->st_atime = node->atime;
	info->st_mtime = node->mtime;
	info->st_ctime = node->ctime;
	
	return 1;
}

/**
 \fn char *VFS_GetTruePath(char *path)
 \brief Evaluates a path, resolving all symbolic links and returns the final path
 \param path	String - Logical path
*/
char *VFS_GetTruePath(char *path)
{
	 int	mnt, longestMnt = -1;
	 int	retLen = 0;
	 int	slashPos, start;
	 int	tmpCheck;
	char	*ret;
	void	*tmpPtr;
	vfs_node	*node, *nextNode;
	
	// Get Mount Point
	for(mnt=0;mnt<mountCount;mnt++)
	{
		tmpCheck = strcmp(path, mounts[mnt].mountPoint);
		// Root of Filesystem
		if(tmpCheck == '\0') {
			if(mounts[mnt].rootNode == NULL) {
				warning("VFS_GetTruePath - %s is mounted incorrectly\n", mounts[mnt].mountPoint);
				return NULL;
			}
			
			ret = malloc( strlen(path) + 1);
			strcpy(ret, path);
			return ret;
		}
		
		// Below the root - Needs to be checked so that a mount on a mount is valid
		if(tmpCheck == '/') {
			if(mounts[mnt].mountPointLen >  mounts[longestMnt].mountPointLen)
				longestMnt = mnt;
		}
	}
	
	if(longestMnt == -1)
		mnt = 0;
	else
		mnt = longestMnt;
	
	retLen = mounts[mnt].mountPointLen;
	ret = malloc(retLen+1);
	memcpy(ret, mounts[mnt].mountPoint, retLen);
	ret[retLen] = '\0';
	start = slashPos = retLen + 1;
	tmpCheck = 0;
	
	// Scan Filesystem Hierachy
	node = mounts[mnt].rootNode;
	while(node)
	{
		if(node->flags & VFS_FFLAG_SYMLINK)
		{
			free(ret);
			ret = VFS_GetTruePath( (char*)node->impl );
			retLen = strlen( (char*)node->impl );
			node = node->link;
			continue;
		}
		if(node->flags & VFS_FFLAG_DIRECTORY)
		{
			while(path[slashPos] && path[slashPos] != '/')	slashPos++;
			if(path[slashPos] == 0)	tmpCheck = 1;	// Mark at end of string
			path[slashPos] = '\0';
			//LogF(" VFS_GetTruePath: path = '%s'\n", path);
			nextNode = node->finddir(node, path+start);
			path[slashPos] = '/';
			//LogF(" VFS_GetTruePath: path = '%s'\n", path);
			slashPos ++;
			start = slashPos;
			if(node->close)	node->close(node);
			node = nextNode;
			
			// NULL Check
			if(node == NULL) {
				free(ret);
				return NULL;
			}
			
			// Append to string
			//LogF(" VFS_GetTruePath: ret = '%s'\n", ret);
			tmpPtr = realloc(retLen+2+node->nameLength, ret);
			if(tmpPtr == NULL) {
				free(ret);
				return NULL;
			}
			ret = tmpPtr;
			ret[retLen] = '/';
			memcpy(ret+retLen+1, node->name, node->nameLength);
			retLen += 1+node->nameLength;
			//LogF(" VFS_GetTruePath: New ret = '%s'\n", ret);
			if(tmpCheck) {
				path[slashPos-1] = '\0';	// Repair String
				ret[retLen] = '\0';	// Cap off string
				break;
			}
		}
		else
		{
			ret[retLen] = '\0';	// Cap off string
			break;
		}
	}
	//LogF("VFS_GetTruePath: RETURN '%s'\n", ret);
	return ret;
}

/*!	\fn int vfs_mount(char *device, char *mountPoint, Uint16 filesystem)
 *	\brief Mounts a device so it can be accessed
 *	\param device		String defining the device to be mounted
 *	\param mountPoint	The place to mount the object
 *	\param filesystem	Specifies what file system should be used
 *
 *	This determines what type of device is going to be mounted and
 *	then binds the device/file to the mount point.
 */
int vfs_mount(char *device, char *mountPoint, Uint16 filesystem)
{
	t_vfs_mount	*mnt = &mounts[mountCount];
	
	#if DEBUG
		LogF("vfs_mount: %s => %s (FS #%x)\n", device, mountPoint, filesystem);
	#endif
	
	if(mountCount == MAX_MOUNTS)
	{
		panic("vfs_mount - Out of Avaliable Mounts\n");
	}
	
	//Get Device Type
	if(device[0] != '/')
		mnt->type = SYS_VIRTUAL;
	else
	{
		if(device[1] == '/')
			mnt->type = SYS_NETWORK;
		else
			mnt->type = SYS_NORMAL;
	}
	
	#if DEBUG
		LogF(" Type: %i\n", mnt->type);
	#endif
	
	//Switch
	switch(mnt->type)
	{
	case SYS_NORMAL:
	case SYS_VIRTUAL:
		if(vfsDrivers[filesystem] == NULL) {
			warning("Mounting %s Failed (Unknown FS 0x%x)", device, filesystem);
			return 0;
		}
		mnt->fileSystem = filesystem;
		mnt->rootNode = (vfsDrivers[filesystem])(device);
		break;
	}
	if(mnt->rootNode == NULL)
	{
		warning("Error Occured while mounting `%s'", device);
		return 0;
	}
	mnt->mountPointLen = strlen(mountPoint);
	memcpy(mnt->mountPoint, mountPoint, mnt->mountPointLen+1);
	memcpy(mnt->device, device, strlen(device)+1);
	
	LogF("[VFS ] Mounted '%s' to '%s'\n", device, mountPoint);
	
	mountCount++;
	return 1;
}

/*! \fn int vfs_mknod(char *path, int isdir);
 *  \brief Creates a new filesystem node
 */
int vfs_mknod(char *path, int isdir)
{
	int pos, pathLen;
	vfs_node	*ret = NULL;
	int	id;
	tFileHandle	*handle;
	
	// Get Length of path
	pathLen = strlen(path);
	// Split Path
	for(pos=pathLen;pos--;) {
		if(path[pos] == '/')	// Check for slash 
		{
			path[pos] = '\0';
			break;
		}
	}
	pos++;
	
	// Check if on root of filesystem
	if(pos == 1)
	{
		if(mounts[0].rootNode->mknod != NULL)
		{
			ret = mounts[0].rootNode->mknod(mounts[0].rootNode, (char*)(path+pos), isdir);
		}
		else
		{
			warning("vfs_mknod - Read-only filesystem (No mknod method)\n");
		}
	}
	else
	{
		handle = vfs_getFileHandle( id = vfs_open(path, 0) );
		if(handle == NULL)
		{
			warning("vfs_mknod - No Such directory\n");
		}
		else
		{
			if(handle->node != NULL)
			{
				ret = handle->node->mknod(handle->node, (char*)(path+pos), isdir);
			}
			else
			{
				warning("vfs_mknod - Read-only filesystem (No mknod method)\n");
			}
			
			vfs_close( id );
		}
	}
	path[pos-1] = '/';
	
	if(ret == NULL)		return 0;
	if(ret->close != NULL)	ret->close(ret);
	return 1;
}

/**
 */
int vfs_ioctl(int fh, int id, void *data)
{
	tFileHandle	*h = vfs_getFileHandle( fh );
	if( h == NULL )	return -1;
	if( mounts[ h->mount ].fileSystem != DEVFS_ID )
		return -1;
	return devfs_ioctl( h->node, id, data );
}

/*
int vfs_checkPerms(vfs_node *node, int req)
- Checks the permissions on a file
*/
int vfs_checkPerms(vfs_node *node, int req)
{
	req &= 7;
	if(node == NULL)	return 0;
	
	if(Proc_GetUid() == 0)	// Superuser / SYSTEM
		return 1;
	
	if(node->uid == Proc_GetUid())
	{
		if( ((node->mode>>6) & req) == req)
			return 1;
		return 0;
	}
	if(node->gid == Proc_GetGid())
	{
		if( ((node->mode>>3) & req) == req)
			return 1;
		return 0;
	}
	if( (node->mode & req) == req)
		return 1;
	return 0;
}

/**
tFileHandle	*vfs_getFileHandle(int id)
- Gets a file handle from a handle id
*/
tFileHandle	*vfs_getFileHandle(int id)
{
	#define VFS_BASE	0xE8000000
	if(id < 0)	return NULL;
	// Kernel File
	if(id & FH_K_FLAG)
	{
		id &= ~FH_K_FLAG;
		if(id > giVfsLastKFile)	return NULL;
		return	&gpKernelHandles[id];
	}
	else
	{
		if(id > giVfsLastFile)	return NULL;
		return	&gpFirstHandle[id];
	}
}

//====================
//= INODE.C
//====================
/*
AcessOS v0.2
Inode Cache
*/

//Constants
#define	INODE_ARR_STEP	16

//Structures
typedef struct sCacheEntry {
	Uint32	handle;
	Uint32	inode;
	int		refCount;
	vfs_node	node;
	struct sCacheEntry	*next;
} cache_entry;

//Globals
 int	inode_nextId = 1;
cache_entry	*inode_cache = NULL;

//Prototypes
int 	inode_initCache();
void	inode_clearCache(int handle);
vfs_node	*inode_getCache(int handle, Uint32 inode);
vfs_node	*inode_cacheNode(int handle, Uint32 inode, vfs_node *node);
void	inode_uncacheNode(int handle, Uint32 inode);

//Code
int inode_initCache()
{
	#if DEBUG
		LogF("inode_initCache: Returning %i\n", inode_nextId);
	#endif
	return inode_nextId++;
}

/* Deallocates all cache entries associated with a handle
 */
void inode_clearCache(int handle)
{
	cache_entry	*ent = inode_cache;
	cache_entry	*tmpEnt;
	// Traverse list
	while(ent && ent->next)	// Itterate
	{
		// Matching
		if(ent->next->handle == handle)
		{
			// And delete
			tmpEnt = ent->next;
			ent->next = tmpEnt->next;
			free(tmpEnt);
		}
		else // Go to next
			ent = ent->next;
	}
	// Handle Currently ignored start of list
	ent = inode_cache;
	if(ent && ent->handle == handle)
	{
		inode_cache = ent->next;
		free(ent);
	}
}

/* Returns a cached node using it's handle and inode number
 */
vfs_node *inode_getCache(int handle, Uint32 inode)
{
	cache_entry	*ent = inode_cache;
	// Traverse list
	while(ent)
	{
		if(ent->handle == handle && ent->inode == inode)
			return &ent->node;
		ent = ent->next;
	}
	return NULL;
}

/* Caches a node
 */
vfs_node *inode_cacheNode(int handle, Uint32 inode, vfs_node *node)
{
	cache_entry	*ent = inode_cache;
	vfs_node	*tmpNode;
	
	#if DEBUG
		LogF("inode_cacheNode: (handle=%i, inode=0x%x)\n", handle, (Uint)inode);
	#endif
	
	// Check if node is already cached
	tmpNode = inode_getCache(handle, inode);
	if( tmpNode != NULL )
	{
		warning("Inode already cached.\n");
		return tmpNode;
	}
	
	// Create New node
	ent = (cache_entry *)malloc(sizeof(cache_entry));
	if(ent == NULL) {
		panic("inode_cacheNode - Out of space for cache table\n");
		return NULL;
	}
	
	#if DEBUG
		LogF(" inode_cacheNode: Caching Node\n");
	#endif
	
	//Cache Node
	ent->handle = handle;
	ent->inode = inode;
	ent->refCount = 1;
	ent->next = inode_cache;	//Pop on the top of the list
	inode_cache = ent;
	memcpy(&ent->node, node, sizeof(vfs_node));
	return &ent->node;
}

/* Remove a node from the cache
 */
void inode_uncacheNode(int handle, Uint32 inode)
{
	cache_entry	*ent = inode_cache;
	cache_entry	*tmpEnt = NULL;
	
	#if DEBUG
		LogF("inode_uncacheNode: (handle=%i, inode=0x%x)\n", handle, inode);
	#endif
	
	while(ent && ent->next)
	{
		if(ent->next->handle == handle && ent->next->inode == inode)
		{
			tmpEnt = ent->next;
			tmpEnt->refCount --;
			if(tmpEnt->refCount > 0)	return;	// Check Reference Count
			ent->next = tmpEnt->next;
			free(tmpEnt);
			return;
		}
		ent = ent->next;
	}
	ent = inode_cache;
	// Handle Currently ignored start of list
	if(!ent && ent->handle == handle && ent->inode == inode)
	{
		ent->refCount --;
		if(ent->refCount > 0)
			return;
		inode_cache = ent->next;
		free(ent);
	}
}


//==============================
//= Exported Function Pointers
//==============================
Uint32	vfs_functions[] = {
	(Uint32)vfs_addfs,	//0
	(Uint32)vfs_open,	//1
	(Uint32)vfs_close,	//2
	(Uint32)vfs_read,	//3
	(Uint32)vfs_write,	//4
	(Uint32)vfs_seek,	//5
	(Uint32)vfs_tell,	//6
	(Uint32)vfs_readdir,	//7
	(Uint32)vfs_mount,	//8
	
	(Uint32)inode_initCache,	//9
	(Uint32)inode_clearCache,	//10
	(Uint32)inode_getCache,		//11
	(Uint32)inode_cacheNode,	//12
	(Uint32)inode_uncacheNode	//13
};

