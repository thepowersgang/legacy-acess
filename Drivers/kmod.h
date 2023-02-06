/*
AcessOS Version 1
Kernel Module Linking
HEADER
*/
#ifndef __KMOD_H
#define __KMOD_H

#define NULL ((void*)0)

typedef unsigned int	Uint;
typedef unsigned long	Uint32;
typedef signed long		Sint32;
typedef unsigned short	Uint16;
typedef signed short	Sint16;
typedef unsigned char	Uint8;
typedef signed char		Sint8;


typedef struct regs {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
}	t_regs;

/**
 \struct vfs_file_info
 \brief Structure for returning information on a file
*/
typedef struct {
	char	name[255];		//!< Filename (duh!)
	Uint32	size;		//!< File Size
	Uint8	flags;		//!< File flags
	Uint32	atime;		//!< Last Accessed Time
	Uint32	mtime;		//!< Last Modified Time
	Uint32	ctime;		//!< Creation Time
	Uint32	uid;
	Uint32	gid;
}	vfs_file_info;

/**
 \struct vfs_node
 \brief Node structure used as a handle
*/
typedef struct s_vfs_node {
	//char	name[256];	//!< Filename
	char	*name;	//!< Filename
	int		nameLength;
	Uint32	inode;	//!< ID Number used by driver
	Uint32	length;	//!< Length
	Uint32	impl;	//!< Extra Variable for driver
	
	Uint32	uid;	//!< User ID
	Uint32	gid;	//!< Group ID
	Uint16	mode;	//!< Mode (Permissions)
	Uint8	flags;	//!< Flags
	Uint32	atime;	//!< Last Accessed Time
	Uint32	mtime;	//!< Last Modified Time
	Uint32	ctime;	//!< Creation Time
	
	int		(*read)(struct s_vfs_node *node, int offset, int length, void *buffer);
	int		(*write)(struct s_vfs_node *node, int offset, int length, void *buffer);
	int		(*close)(struct s_vfs_node *node);
	struct s_vfs_node*	(*readdir)(struct s_vfs_node *node, int dirPos);
	struct s_vfs_node*	(*finddir)(struct s_vfs_node *node, char *filename);
	struct s_vfs_node*	(*mknod)(struct s_vfs_node *node, char *filename, int isdir);
	int		(*unlink)(struct s_vfs_node *node);
}	vfs_node;

/**
 \struct t_vfs_driver
 \brief Structure containing addresses to the functions provided by a filesystem driver
*/
typedef struct {
	int		loaded;		//!< Is this driver loaded
	int		(*loadDisk)(char *device);	//!< Read disk information to prepare for reading
	int		(*read)(int handle, int fHandle, int offset, int length, void *buffer);	//!< Read data from disk
	int		(*write)(int handle, int fHandle, int offset, int length, void *buffer);	//!< Write data to a file
	int		(*readdir)(int handle, int dirHandle, int dirPos, char *filename);	//!< Read directory
	int		(*fileinfo)(int handle, char *path, vfs_file_info *file);	//!< Get information on a file
	int		(*newfile)(int handle, char *path, int isdir);	//!< Create a new file
}	t_vfs_driver;

typedef struct {
	char	name[32];
	int		nameLen;
	vfs_node	*rootNode;
	int 	(*ioctl)(vfs_node *node, int id, void *data);
} devfs_driver;

//=== PROTOTYPES ===
//--- Library Functions ---
extern void	memcpy(void *dest, const void *src, Uint32 count);
extern void	memcpyd(void *dst, void *src, Uint32 dwCount);
extern void	*memset(void *dest, char val, Uint16 count);
extern void	*memsetw(void *dest, Uint16 val, Uint16 count);
extern int	strlen(const char *str);
extern int	strcmp(const char *str1, const char *str2);
extern int	strncmp(const char *str1, const char *str2);
extern Uint8  inb(Uint16 _port);
extern Uint16 inw(Uint16 _port);
extern Uint32 ind(Uint16 _port);
extern void	outb(Uint16 _port, Uint8 _data);
extern void	outw(Uint16 _port, Uint16 _data);
extern void	outd(Uint16 _port, Uint32 _data);
//--- DevFS ---
extern int	DevFS_AddDevice(devfs_driver *drv);
extern void DevFS_DelDevice(int drv);
extern int	DevFS_AddSymlink(int drv, int handle, char *name);
extern void DevFS_DelSymlink(int id);
//--- VFS ---
extern int	VFS_AddFS(Uint16 ident, vfs_node* (*initDev)(char *device));
extern int	VFS_Open(char *path, int flags);
extern void	VFS_Close(int fileHandle);
extern int	VFS_Read(int fileHandle, int length, void *buffer);
extern int	VFS_Write(int fileHandle, int length, void *buffer);
extern void	VFS_Seek(int fileHandle, int distance, int flag);
extern int	VFS_Tell(int fileHandle);
extern int	VFS_ReadDir(int fileHandle, char *name);
extern int	VFS_Mount(char *device, char *mountPoint, Uint8 filesystem);
//--- Inode Cache ---
extern int	Inode_InitCache(void);
extern void Inode_ClearCache(int handle);
extern vfs_node *Inode_GetCache(int handle, Uint32 inode);
extern vfs_node *Inode_CacheNode(int handle, Uint32 inode, vfs_node *node);
extern void Inode_UncacheNode(int handle, Uint32 inode);
//--- Memory Manager Functions ---
extern void	*MM_MapHW(Uint32 paddr, Uint count);
extern void	*MM_UnmapHW(Uint32 paddr, Uint count);
//--- Misc Functions ---
extern int	now();
extern int	timestamp(int sec, int mins, int hrs, int day, int month, int year);
extern void	*malloc(int bytes);
extern void	free(void *mem);
extern void	*realloc(int bytes, void *mem);
extern void	DMA_SetRW(int channel, int read);
extern int	DMA_ReadData(int channel, int count, void *buffer);
extern int	Time_CreateTimer(float delay, void(*callback)(int), int arg, int oneshot);
extern void	Time_RemoveTimer(int id);
extern void	IRQ_Set(int irq, void (*handler)(t_regs *r));
extern void	IRQ_Clear(int irq);

#define VFS_FFLAG_READONLY	0x01
#define VFS_FFLAG_HIDDEN	0x02
#define VFS_FFLAG_DIRECTORY	0x10

#endif /*define(__KMOD_H) */
