/**
 Acess Version 1
 \file drv_usbuhci.c
 \brief USB UHCI Driver
*/
#define MODULE_ID	UHCI
#include <kmod.h>
#include <drv_usbuhci.h>

// === PROTOTYPES ===
 int	ModuleLoad();
void	ModuleUnload();
 int	UHCI_IOCtl(vfs_node *node, int id, void *data);

// === GLOBAS ===
char	*ModuleIdent = #MODULE_ID;
devfs_driver	gUHCI_DrvInfo = {
	{0}, UHCI_IOCtl
};
Uint	gaFrameList[1024];
tController	gUhciControllers[4];

// === CODE ===
/**
 */
int ModuleLoad()
{
	 int	id=-1, count;
	while( (id = PCI_GetDeviceByClass(0x0C03, 0xFFFF, id)) >= 0 )
	{
		base = PCI_AssignPort( id, 4, 0x20 );	// Assign a port (BAR4, Reserve 32 ports)
	}
	return 0;
}

void ModuleUnload()
{
}

/**
 * \fn int	UHCI_IOCtl(vfs_node *node, int id, void *data)
 */
int	UHCI_IOCtl(vfs_node *node, int id, void *data)
{
	return 0;
}
