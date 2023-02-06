/*
AcessOS 0.1
Core Common System Functions
*/
#include <acess.h>
#include <keysym.h>

// ==== GLOBALS ====
Uint32 giMemoryCount = 0;
char	*gsNullString = "";

// ==== IMPORTS ====
extern void Proc_DumpRunning();
extern void MM_DumpUsage();
extern Uint	time_accurate, time_msWhole;

// ==== PROTOTYPES ====
void System_Restart();

// ==== CODE ====
void Kernel_KeyEvent( Uint8 *gbaKeystates, Uint8 keycode )
{
	if((gbaKeystates[KEY_LCTRL] || gbaKeystates[KEY_RCTRL])
	 && (gbaKeystates[KEY_LALT] || gbaKeystates[KEY_RALT])
	 && gbaKeystates[KEY_ESC]
		)
	{
		switch(keycode)
		{
		// Print Current Timestamp
		case 't':
			LogF("Current Timestamp: %u.%03u\n", time_accurate, time_msWhole);
			return;
		// Print Current Timestamp
		case 'p':
			Proc_DumpRunning();
			return;
		// Dump Memory Usage
		case 'm':
			MM_DumpUsage();
			return;
		// Restart System Forcefully
		case 'r':
			System_Restart();
			return;
		}
	}
}

void System_Restart()
{
	warning("Acess is restarting...\n");
	IDT_SetGate(8, 0, 0, 0);	// Clear Double Fault ISR
	IDT_SetGate(13, 0, 0, 0);	// Clear GP Fault ISR
	__asm__ __volatile__ ("int $8");
}

void System_Init()
{
	
}
