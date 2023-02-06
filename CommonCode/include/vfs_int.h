/*
AcessOS/AcessBasic v0.1
VFS Internal Header File

VFS_INT.H
*/

#ifndef _VFS_INT_H

#define IS_HANDLE(h)	((IS_ACESS_OS&&(\
	(0xE0000000<=(Uint)(h))&&((Uint)(h)<0xF0000000)\
	&&((tFileHandle*)(h))->mount>=0&&((tFileHandle*)(h))->mount<MAX_MOUNTS))\
	||(!IS_ACESS_OS && (Uint)(h) < 0x10000000))
#define VALIDATE_HANDLE(h, ret)	\
	do {if(!IS_HANDLE(h)) {\
			warning("%s - Bad File Handle Passed (0x%x)0x%x\n", __FUNCTION__, (h), -1);\
			return ret;\
	}}\
	while(0)

#define VFS_OPENFLAG_MASK	7
	
//STRUCTURES
/**
 \enum SystemTypes
 \brief Enum of mount types
*/
enum SystemTypes {
	SYS_NULL,
	SYS_VIRTUAL,
	SYS_NORMAL,
	SYS_NETWORK
};
/**
 \struct t_vfs_mount
 \brief Structure for mount information
*/
typedef struct {
	int		type;		//!< Mount Type - Use SystemTypes
	char	device[256];	//!< Device Path
	int		mountPointLen;	//!< Length of mountPoint - removes need for multiple strlen calls
	char	mountPoint[256];	//!< Path the device/image is mounted at
	Uint16	fileSystem;		//!< File System of mounted device
	vfs_node	*rootNode;	//!< Root Node of Mount
} t_vfs_mount;
/**
 \struct t_file_handle
 \brief Structure for storing file/directory handles
*/
typedef struct sFileHandle {
	Sint16	mount;	//!< Mount ID
	Uint16	mode;	//!< Open Mode
	int		pos;	//!< File Position
	vfs_node	*node;	//!< Node of open file
	struct sFileHandle	*next;	//!< Next File in list
} tFileHandle;

/**
 \typedef t_fs_initDev
 \brief Type of function pointer used to initialise a mount
*/
typedef vfs_node* (*t_fs_initDev)(char* device);

#endif
