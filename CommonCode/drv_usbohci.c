/**
 Acess Version 1
 \file drv_usbohci.c
 \brief USB OHCI Driver
*/
#define MODULE_ID	USBOHCI
#include <kmod.h>

// === CONSTANTS ===

// === PROTOTYPES ===
 int	ModuleLoad();
void	ModuleUnload();

// === GLOBAS ===
char	*ModuleIdent = #MODULE_ID;

// === CODE ===
/**
 */
int ModuleLoad()
{
	return 0;
}

void ModuleUnload()
{
}
