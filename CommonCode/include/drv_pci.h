/*
AcessOS/AcessBasic v0.1
PCI Bus Driver

DRV_PCI.H
*/
#ifndef _DRV_PCI_H
#define _DRV_PCI_H

enum e_PciClasses {
	PCI_CLASS_PRE20 = 0x00,
	PCI_CLASS_STORAGE,
	PCI_CLASS_NETWORK,
	PCI_CLASS_DISPLAY,
	PCI_CLASS_MULTIMEDIA,
	PCI_CLASS_MEMORY,
	PCI_CLASS_BRIDGE,
	PCI_CLASS_COMM,
	PCI_CLASS_PREPH,
	PCI_CLASS_INPUT,
	PCI_CLASS_DOCKING,
	PCI_CLASS_PROCESSORS,
	PCI_CLASS_SERIALBUS,
	PCI_CLASS_MISC = 0xFF
};
enum e_PciOverClasses {
	PCI_OC_PCIBRIDGE = 0x0604,
	PCI_OC_SCSI = 0x0100
};

#endif
