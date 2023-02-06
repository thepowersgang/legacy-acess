/*
AcessOS 0.01
Keyboard Driver

Handles Keyboard Interrupts and sets global
variable `kb_lastChar` for tty driver.
*/
#include <acess.h>

// === DEFINES ===
//Special Keys
#define	KB_LF_SHIFT	42
#define	KB_RT_SHIFT	54
#define	KB_LF_CTRL	29
#define	KB_RT_CTRL	0
#define	KB_LF_ALT	56
#define	KB_RT_ALT	0

// === GLOBALS ===
Uint16 kbdus[128] = {
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',	/* 14 - Backspace */
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',		/* 28 - Enter key */
	0, /* 29   - Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, /* 42 - Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, /* 54 - Right shift */
	'*',
	0,	/* 56 - Alt */
	' ',	/* Space bar */
	0,	/* 58 - Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/**
* \var KB_keystates
* \brief Flags relating to keyboard key states
* 
* This is a bitmap with 1 bit indicating if a
* key is up or down
*/
Uint32	KB_keystates[4];
Uint8	KB_isShift = 0;
int		kb_lastChar = -1;


// === CODE ==
/**
 \fn void keyboard_handler(struct regs *r)
 \brief Handles the keyboard interrupt
 \param r Required for an IRQ handler. System registers at IRQ call
*/
void keyboard_handler(struct regs *r)
{
    unsigned char scancode;
	char	ch;

    scancode = inportb(0x60); // Read from the keyboard's data buffer
	
	putHex(scancode);
	putch(' ');
	
    /* If the top bit of the byte we read from the keyboard is
     *  set, that means that a key has just been released */
    if (scancode & 0x80)
    {
		scancode = scancode & 0x7F;	//Get Scancode
		KB_keystates[ scancode>>5 ] &= !(1 << (scancode&0x1F));		//Unset the bit relating to the key
		
		if(scancode == KB_RT_SHIFT || scancode == KB_LF_SHIFT)		//Is the key a shift
			KB_isShift = 0;		//Set Shift to released
    }
    else
    {
		KB_keystates[scancode>>5] |= 1 << (scancode&0x1F);		//Set the bit relating to the key
		if(scancode == KB_RT_SHIFT || scancode == KB_LF_SHIFT)
			KB_isShift = 1;
		
		ch = kbdus[scancode];
		if(ch == 0)
			return;
		//Is shift pressed
		if(KB_isShift) {
			switch(ch) {
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
		kb_lastChar = ch;
    }
}

/**
 \fn void keyboard_install()
 \brief Installs keyboard interrupt (IRQ1)
*/
void keyboard_install()
{
    irq_install_handler(1, keyboard_handler);
}
