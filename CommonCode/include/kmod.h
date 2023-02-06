/*
Acess OS
Kernel Module Compatability File
HEADER
*/
#ifndef __KMOD_H
#define __KMOD_H

// Check if a Module ID is present
#ifndef MODULE_ID
# error "A Module ID is required, Please define MODULE_ID before including kmod.h"
#endif

// Required Includes
#include <acess.h>
#include <vfs.h>
#include <fs_devfs2.h>

// Adjust Module Identifiers
#define ModuleIdent		EXPAND_CONCAT(MODULE_ID,_ModuleIdent)
#define ModuleLoad		EXPAND_CONCAT(MODULE_ID,_Load)
#define ModuleUnload	EXPAND_CONCAT(MODULE_ID,_Unload)


// Helper Functions
#define	inb(p)	inportb(p)
#define	inw(p)	inportw(p)
#define	ind(p)	inportd(p)
#define	outb(p,v)	outportb(p, v)
#define	outw(p,v)	outportw(p, v)
#define	outd(p,v)	outportd(p, v)

#define DMA_ReadData(v...)	dma_readData(v)
#define DMA_SetChannel(v...) 	dma_setChannel(v)

#define Time_CreateTimer(v...)	time_createTimer(v)
#define Time_RemoveTimer(v...)	time_removeTimer(v)

#define DevFS_AddDevice(v...)	dev_addDevice(v)

#define IRQ_Set(v...)	irq_install_handler(v)
#define IRQ_Clear(v...)	irq_uninstall_handler(v)

#endif
