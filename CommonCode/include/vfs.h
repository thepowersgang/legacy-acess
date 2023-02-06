/*
Acess OS
Virtual File System Version 1
HEADER
*/
#ifndef _VFS_H
#define _VFS_H
/**
 \file vfs.h
 \brief Virtual Filesystem Interface
*/


#define MAX_PATH_LENGTH	255	//!< Maximum Path Length

#define VFS_FFLAG_READONLY	0x01	//!< Readonly Node Flag
#define VFS_FFLAG_HIDDEN	0x02	//!< Hidden Node Flag
#define VFS_FFLAG_DIRECTORY	0x10	//!< Directory Flag
#define VFS_FFLAG_SYMLINK	0x20	//!< Symbolic Link Flag

/**
 \struct s_vfs_node
 \brief Node information structure
 
 The type #vfs_node is the core of the Acess VFS.
*/
struct s_vfs_node {
	char	*name;	//!< Filename
	 int	nameLength;	//!< Length of name
	Uint	inode;	//!< ID Number used by driver (Also used to store symlink target)
	Uint	length;	//!< Length
	Uint	impl;	//!< Extra Variable for driver
	
	Uint32	uid;	//!< User ID
	Uint32	gid;	//!< Group ID
	Uint16	mode;	//!< Mode (Permissions)
	Uint8	flags;	//!< Flags
	Uint32	atime;	//!< Last Accessed Time
	Uint32	mtime;	//!< Last Modified Time
	Uint32	ctime;	//!< Creation Time
	
	//! Read from Node
	int		(*read)(struct s_vfs_node *node, int offset, int length, void *buffer);
	//! Write to Node
	int		(*write)(struct s_vfs_node *node, int offset, int length, void *buffer);
	//! Close Node
	int		(*close)(struct s_vfs_node *node);
	//! Read from Directory
	struct s_vfs_node*	(*readdir)(struct s_vfs_node *node, int dirPos);
	//! Find an item in a Directory
	struct s_vfs_node*	(*finddir)(struct s_vfs_node *node, char *filename);
	//! Make a node in a Directory
	struct s_vfs_node*	(*mknod)(struct s_vfs_node *node, char *filename, int flags);
	//! Remove the node
	int		(*unlink)(struct s_vfs_node *node);
	
	struct s_vfs_node*	link;	//!< Link for symbolic links
};
typedef struct s_vfs_node	vfs_node;	//!< VFS Node Type


#define ROOTFS_ID	0x1AC	//!< RootFS Fileystem ID
#define DEVFS_ID	0x1AD	//!< DevFS Filesystem ID
#define INITROMFS_ID	0x1AE	//!< InitRomFS Fileystem ID

/* VFS.C */
/**
 \var vfs_node	NULLNode
 \brief Node equivilent to ""
 
 This is a VFS Node that is used as a placeholder
 for an empty directory entry by vfs_node.readdir
*/
extern vfs_node	NULLNode;

/**
 \fn int vfs_addfs(Uint16 ident, vfs_node* (*initDev)(char* device))
 \brief Adds a filesystem to the VFS Layer
 \param ident	Filesystem identifier
 \param initDev	Function pointer to mount initialiser for filesystem
*/
extern int vfs_addfs(Uint16 ident, vfs_node* (*initDev)(char* device));

/**
 \fn extern int vfs_removefs(Uint16 ident)
 \brief Removes a filsystem from the VFS layer
 \param ident	Identifier of FS to remove
*/
extern int vfs_removefs(Uint16 ident);

/* INODE.C */
/**
 \fn int inode_initCache()
 \brief Creates a handle for a filesystem
 \return Handle
*/
extern int	inode_initCache();

/**
 \fn void inode_clearCache(int handle)
 \brief Clear the cache of all a driver's entries
 \param handle	Driver/Instance handle
*/
extern void	inode_clearCache(int handle);

/**
 \fn int inode_isCached(int handle, Uint32 inode)
 \brief Checks if a node is cached
 \param handle	Cache Handle
 \param inode		Inode ID to check
*/
extern int	inode_isCached(int handle, Uint32 inode);

/**
 \fn vfs_node *inode_getCache(int handle, Uint32 inode)
 \brief Gets a node from the cache
 \param handle	Cache Handle
 \param inode		Inode ID
*/
extern vfs_node	*inode_getCache(int handle, Uint32 inode);

/**
 \fn vfs_node *inode_cacheNode(int handle, Uint32 inode, vfs_node *node)
 \brief Gets a node from the cache
 \param handle	Cache Handle
 \param inode		Inode ID
 \param node		Node pointer to copy from
*/
extern vfs_node	*inode_cacheNode(int handle, Uint32 inode, vfs_node *node);

/**
 \fn void inode_uncacheNode(int handle, Uint32 inode)
 \brief Removes a node from the cache
 \param handle	Cache Handle
 \param inode		Inode ID
*/
extern void	inode_uncacheNode(int handle, Uint32 inode);

#endif
