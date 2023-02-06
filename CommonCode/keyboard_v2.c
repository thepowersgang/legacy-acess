/*
AcessOS 0.01
Keyboard Driver

Handles Keyboard Interrupts and sets global
variable `kb_lastChar` for tty driver.
*/
#include <acess.h>
#include <keysym.h>
#include <drivers/kbdus.h>
#include <drivers/kbdus2.h>

// === GLOBALS ===
Uint8	**gpKeyMap = gpKBDUS;
//Uint32	gbaKeystates_[8];	//0-256
Uint8	gbaKeystates[256];
int		gbKbShiftState = 0;
int		gbKbCapsState = 0;
int		kb_lastChar = -1;
int		gbKeyUp = 0;
int		giKeyLayer = 0;


// === CODE ==
void Keyboard_Handler(struct regs *r)
{
	Uint8	scancode;
	Uint8	ch;

	if(inportb(0x64) & 0x20)	return;
	
	scancode = inportb(0x60); // Read from the keyboard's data buffer

	if(scancode == 0xFA)
	{
		kb_lastChar = KB_ACK;
		return;
	}
	
	if(scancode == 0xE0)
	{
		giKeyLayer = 1;
		return;
	}
	if(scancode == 0xE1)
	{
		giKeyLayer = 2;
		return;
	}
	
	#if KB_ALT_SCANCODES
	if(scancode == 0xF0)
	{
		gbKeyUp = 1;
		return;
	}
	#else
	if(scancode & 0x80)
	{
		scancode &= 0x7F;
		gbKeyUp = 1;
	}
	#endif
	
	ch = gpKeyMap[giKeyLayer][scancode];
	if(!ch && !gbKeyUp){puts("UNK ");putNum(giKeyLayer);putch(' ');putHex(scancode);putch(' ');}
	giKeyLayer = 0;
	
	if (gbKeyUp)
	{
		gbKeyUp = 0;
		//gbaKeystates[ ch>>5 ] &= !(1 << (ch&0x1F));	// Unset the bit relating to the key
		gbaKeystates[ ch ] = 0;	// Unset key state flag
		
		//if( !(gbaKeystates[KEY_LSHIFT>>5] & (1 << (KEY_LSHIFT&0x1F)))
		// && !(gbaKeystates[KEY_RSHIFT>>5] & (1 << (KEY_RSHIFT&0x1F))) )
		if( !gbaKeystates[KEY_LSHIFT] && !gbaKeystates[KEY_RSHIFT] )
			gbKbShiftState = 0;
		
		return;
	}

	// Set the bit relating to the key
	//gbaKeystates[ch>>5] |= 1 << (ch&0x1F);
	gbaKeystates[ch] = 1;
	if(ch == KEY_LSHIFT || ch == KEY_RSHIFT)
		gbKbShiftState = 1;
		
	if(ch == KEY_CAPSLOCK)
		gbKbCapsState = !gbKbCapsState;

	// Ignore Non-Printable Characters
	if(ch == 0 || ch & 0x80)		return;
		
	// Is shift pressed
	if(gbKbShiftState ^ gbKbCapsState)
	{
		switch(ch)
		{
		case '`':	ch = '~';	break;
		case '1':	ch = '!';	break;
		case '2':	ch = '@';	break;
		case '3':	ch = '#';	break;
		case '4':	ch = '$';	break;
		case '5':	ch = '%';	break;
		case '6':	ch = '^';	break;
		case '7':	ch = '&';	break;
		case '8':	ch = '*';	break;
		case '9':	ch = '(';	break;
		case '0':	ch = ')';	break;
		case '-':	ch = '_';	break;
		case '=':	ch = '+';	break;
		case '[':	ch = '{';	break;
		case ']':	ch = '}';	break;
		case '\\':	ch = '|';	break;
		case ';':	ch = ':';	break;
		case '\'':	ch = '"';	break;
		case ',':	ch = '<';	break;
		case '.':	ch = '>';	break;
		case '/':	ch = '?';	break;
		default:
			if('a' <= ch && ch <= 'z')
				ch -= 0x20;
			break;
		}
	}
	
	Kernel_KeyEvent(gbaKeystates, ch);
	
	kb_lastChar = ch;
	//LogF("Keyboard_Handler: ch='%c'(0x%x)\n", ch, ch);
}

/**
 \fn void keyboard_install()
 \brief Installs keyboard interrupt (IRQ1)
*/
void Keyboard_Install()
{
    irq_install_handler(1, Keyboard_Handler);
}
