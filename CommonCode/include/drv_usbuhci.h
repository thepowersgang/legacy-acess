/**
 */

#ifndef _DRV_USBUHCI_H
#define _DRV_USBUHCI_H

// === TYPES ===
typedef struct sController {
} tController;

// === CONSTANTS ===
enum eUhciPorts {
	/* USB Control / Command
	 * 15:8 - Unused
	 * 7 - Max Packet Size (0: 32B, 1: 64B)
	 * 6 - CF, Configure Flag (Set at end of config)
	 * 5 - Software Debug (1: Enabled, 0: Disabled)
	 * 4 - Force Global Resume
	 * 3 - Enter Global suspend mode
	 * 2 - Global Reset - Reset to post-poweron state
	 * 1 - Host Controller Reset - 
	 * 0 - Run/Stop - Execution Control (1: Running, 0: Complete current transaction and halt)
	 */
	USBCMD, USBCMD_L,	// WORD Only
	/* USB Status
	 * 15:6 - Unused
	 * 5 - HCHalted - Set when stopped due to USBCMD:0 set to 0 or an internal error
	 * 4 - Host Controller Process Error - Set on an internal error
	 * 3 - Host System Error - Set on a PCI Error
	 * 2 - Resume Detect - Set when a RESUME message is recieved from a device
	 * 1 - USB Error Interrupt - Set on a device error
	 * 0 - USB Interrupt - Set when an interrupt is caused by a completed transaction
	 */
	USBSTS, USBSTS_L,	// WORD Only
	/* USB Interrupt Enable
	 * 15:4 - Unused
	 * 3 - Short Packet Detect Enable
	 * 2 - Interrupt On Complete (EOC) Enable
	 * 1 - Resume Interrupt Enable
	 * 0 - Timeout/CRC Fail Interrupt Enable
	 */
	USBINTR, USBINTR_L,	// WORD Only
	/* Frame List Index
	 * 15:11 - Unused / Reserved
	 * 10:0 - Index
	 */
	FRNUM, FRNUM_L,		// WORD Only
	/* Physical Address of Frame List
	 */
	FLBASEADD, FLBASEADD_1, FLBASEADD_2, FLBASEADD_3,	// DWORD
	/* Start of Frame Modify Register
	 * 7 - Reserved
	 * 6:0 - SOF Timing Value - Number of (11936+x) 12MHz cycles
	 */
	SOFMOD,		// BYTE
	/* Port Status and Control - Port 1
	 * 15:13 - Unused
	 * 12 - Suspend (1: In Suspend State, 0: Not Suspended)
	 * 11:10 - Reserved
	 * 9 - Port Reset (1: Resetting, 0: Not Resetting)
	 * 8 - Low Speed Device Attached
	 * 7 - Unused
	 * 6 - Resume Detect
	 * 5:4 - Line Status
	 * 3 - Port Enable/Disable Detect
	 * 2 - Port Enabled/Disabled
	 * 1 - Connect Status Change (1: Change has Occured, reset by writing 1)
	 * 0 - Current Connect Status (1: Device Connected)
	 */
	PORTSC1_H = 0x10, PORTSC1_L,	// WORD Only
	PORTSC2_H, PORTSC2_L,	// WORD Only
};

#endif
