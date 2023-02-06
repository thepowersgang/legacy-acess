/**
 AcessOS v1
 \file fs_devfs2.h
 \brief Device Filesystem Version 2
*/
#ifndef _FS_DEVFS2_H
#define _FS_DEVFS2_H

#include <vfs.h>

//STRUCTURES
/**
 \struct s_devfs_driver
 \brief DevFS Driver Info Structure
*/
struct s_devfs_driver {
	vfs_node	rootNode;	//!< Root Node
	int 	(*ioctl)(vfs_node *node, int id, void *data);	//!< IOCtl Function
};
typedef struct s_devfs_driver	devfs_driver;	//!< Driver Information Type

//DEFINES
#define MAX_DRIVERS 32	//!< Maximum Simultanious Loaded Drivers
#define MAX_LINKS 16	//!< Maximum number of links

//PROTOTYPES
/**
 \fn void devfs_install()
 \brief Initialises and Registers DevFS
*/
extern void devfs_install();

/**
 \fn int dev_addDevice(devfs_driver *drv)
 \brief Registers a driver with DevFS
 \param drv	Driver Information Pointer
 \return Driver ID
*/
extern int	dev_addDevice(devfs_driver *drv);

#endif
