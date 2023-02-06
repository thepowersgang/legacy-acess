/*
AcessOS 0.1
Screen Output
*/
#include <acess.h>

#define ATTR_DEFAULT	0x0F

// === IMPORTS ===
extern void	Proc_DumpRunning();

// === GLOBALS ===
Uint16	*textmemptr;		//!< VGA Buffer Pointer
Uint16	giAttrib = ATTR_DEFAULT << 8;		//!< Font Attributes
Uint16	giTmpAttrib = ATTR_DEFAULT << 8;		//!< Font Attributes
int		giCsrPos = 0;	//!< Cursor Position

char hexDigits[] = "0123456789ABCDEF";

void write_serial(char a);
void init_serial();

// === CODE ===
/**
 \fn void scrn_scroll(void)
 \brief Scrolls the screen
*/
void scrn_scroll(void)
{
    Uint16	blank, temp;

    blank = 0x20 | giAttrib;

	//Do we have to scroll?
    if(giCsrPos >= 25*80)
    {
		//Copy all but the first line upwards on the screen
        temp = giCsrPos/80 - 25 + 1;
        memcpyd (textmemptr, textmemptr + temp * 80, (25 - temp) * 80 / 2);
        //Clear the last line
        memsetw (textmemptr + (25 - temp) * 80, blank, 80);
		giCsrPos = giCsrPos%80 + 24*80;
    }
}

/**
 \fn void scrn_update_csr(void)
 \brief Updates hardware cursor
*/
void scrn_update_csr(void)
{
    outportb(0x3D4, 14);
    outportb(0x3D5, giCsrPos >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, giCsrPos);
}

/**
 \fn void scrn_cls(void)
 \brief Clears the screen
*/
void scrn_cls(void)
{
    unsigned blank;
    int i;
	
    blank = 0x20 | giAttrib;

	memsetw (textmemptr, blank, 80*25);
		
	for(i=80;i--;)	write_serial('=');
	write_serial('\n');

	giCsrPos = 0;
    scrn_update_csr();
}

/**
 \fn void putch(char c)
 \brief Print a character to the screen
 \param c	character - Character to print
*/
void putch(char c)
{
    Uint16	att = giAttrib;
	 int	tmp;
	
	switch(c)
	{
	//Backspace (Move X back and wipe space)
	case 0x08:
		giCsrPos --;
		tmp = giCsrPos;
		textmemptr[tmp] = 0x20 | att;
		tmp --;
		// Eliminate Tabs
		while( (textmemptr[tmp] & 0xFF) == 0 && tmp >= 0)
		{
			tmp --;
		}
		tmp ++;
		giCsrPos = tmp;
		break;
    //Tab (Increase X to next divisor of 8)
    case 0x09:
		tmp = giCsrPos;
        giCsrPos = (giCsrPos + 8) & ~(8 - 1);
		memsetw(&textmemptr[tmp], (0x0 | (att<<8)), tmp&7);
		break;
	//Carrage Return (Set X to 0)
    case '\r':
		giCsrPos -= giCsrPos%80;
		break;
	//Newline - Use as BOIS/Unix Does (Set X to 0 and Inc Y)
    case '\n':
		giCsrPos -= giCsrPos%80;
		giCsrPos += 80;
		break;
	//Printable Character
    default:
		if(c >= ' ')
		{
			textmemptr[giCsrPos] = c | att;	/* Character AND attributes: color */
			giCsrPos ++;
		}
		break;
	}
	
	write_serial(c);

    //Update scroll and cursor
    scrn_scroll();
    scrn_update_csr();
}

/*!
 * \fn void puts(char *text)
 * \brief Prints a string to the screen
 * \param text Null terminiated string to print
 */
void puts(char *text)
{
	while(*text)	putch(*text++);
}

/*!
 * \fn void putsa(char attr, char *text)
 * \brief Prints a string to the screen with the specified attributes
 * \param attr	VGA Attribute Byte
 * \param text	Null terminiated string to print
 */
void putsa(char attr, char *text)
{
	char oldAttr;

	oldAttr = giAttrib;
	giAttrib = attr << 8;
	
	while(*text)	putch(*text++);
	
	giAttrib = oldAttr;
}

/* Sets the forecolor and backcolor that we will use */
void scrn_settextcolor(unsigned char forecolor, unsigned char backcolor)
{
    giAttrib = ((backcolor << 4) | (forecolor & 0x0F)) << 8;
}

/* Sets attributes directly
 */
void VGAText_SetAttrib(Uint8 att)
{
	giAttrib = att << 8;
}

/*
void init_video(void)
- Initialises Video Pointer
- Initialises Serial
- Clears Screen
*/
void VGAText_Install(void)
{
	if((Uint) &textmemptr > 0xC0000000)
		textmemptr = (Uint16 *)0xC00B8000;
	else
		textmemptr = (Uint16 *)0xB8000;
	init_serial();
    scrn_cls();
}

static const char cUCDIGITS[] = "0123456789ABCDEF";
void itoa(char *buf, Uint num, Uint base, int minLength, char pad)
{
	char tmpBuf[32];
	int pos=0,i;

	if(!buf)	return;
	if(base > 16) {
		buf[0] = 0;
		return;
	}
	
	while(num > base-1) {
		tmpBuf[pos] = cUCDIGITS[ num % base ];
		num = (long) num / base;		//Shift {number} right 1 digit
		pos++;
	}

	tmpBuf[pos++] = cUCDIGITS[ num % base ];		//Last digit of {number}
	i = 0;
	minLength -= pos;
	while(minLength-- > 0)	buf[i++] = pad;
	while(pos-- > 0)		buf[i++] = tmpBuf[pos];	//Reverse the order of characters
	buf[i] = 0;
}

/*
void printfv(const char *format, void **args)
- Print Formatted with a variable argument list
*/
void printfv(const char *format, void **args)
{
	char	**arg = (char **) args;
	char	c, pad = ' ';
	 int	minSize = 0, i = 0;
	char	tmpBuf[34];	// For Integers
	char	*p = NULL;
  
	while((c = *format++) != 0)
	{
		if (c != '%')	putch(c);
		else
		{          
			c = *format++;
			i = *((int *) arg++);
			// Padding
			if(c == '0') {
				pad = '0';
				c = *format++;
			} else
				pad = ' ';
			minSize = 0;
			if('1' <= c && c <= '9')
			{
				while('0' <= c && c <= '9')
				{
					minSize *= 10;
					minSize += c - '0';
					c = *format++;
				}
			}
			p = tmpBuf;
			switch (c) {
			case 'd':
			case 'i':
				if(i < 0) {
					putch('-');
					i = -i;
				}
				itoa(p, i, 10, minSize, pad);
				goto printString;
			case 'u':
				itoa(p, i, 10, minSize, pad);
				goto printString;
			case 'x':
				itoa(p, i, 16, minSize, pad);
				goto printString;
			case 'o':
				itoa(p, i, 8, minSize, pad);
				goto printString;
			case 'b':
				itoa(p, i, 2, minSize, pad);
				goto printString;

			case 'B':	//Boolean
				if(i)	puts("True");
				else	puts("False");
				break;
				
            case 's':
				p = (char*)i;
			printString:
				if(!p)		p = "(null)";
				while(*p)	putch(*p++);
				break;

			case '%':
				arg--;
				putch('%');
				break;
				
			default:	putch(i);	break;
            }
        }
    }
}

/*
void LogF(const char *format, ...)
- Print Formatted
*/
void LogF(const char *format, ...)
{
	printfv(format, (void**)((Uint)(&format)+4));
}

void panic(char *msg, ...)
{
	giAttrib = ((CLR_RED|CLR_FLAG)) << 8;
	puts("\nPANIC: ");
	printfv(msg, (void**)((Uint)(&msg)+4));
	
	Proc_DumpRunning();
	
	Timer_Disable();
	__asm__ __volatile__ ("cli");
	for(;;)	__asm__ __volatile__ ("hlt");
}

void warning(char *msg, ...)
{
	giTmpAttrib = giAttrib;
	giAttrib = (CLR_YELLOW|CLR_FLAG)<<8;
	puts("WARNING: ");
	printfv(msg, (void**)((Uint)(&msg)+4));
	giAttrib = giTmpAttrib;
}

/*
void putNumS(long number)
- Print Signed Decimal
*/
void putNumS(long number)
{
	//Init Variables
	char num[10];
	int pos=0;
	//Handle Negatives
	if(number < 0) {
		putch('-');
		number = 0-number;
	}
	//Loop Digits
	while(number > 9) {
		num[pos] = '0' + number % 10;	//'0' + last digit of {number}
		pos++;
		number = (long) number / 10;		//Shift {number} right 1 digit
	}
	
	num[pos] = 0x30 + number % 10;		//'0' + last digit of {number}
	for(;pos>=0;pos--) putch(num[pos]);	//Reverse the order of characters
}

// == putNum ==
// Print Unsigned Decimal
void putNum(unsigned long number)
{
	//Init Variables
	char num[10];	//32 Bit
	int pos=0;
	//Loop Digits
	while(number > 9) {
		num[pos] = '0' + number % 10;	//'0' + last digit of {number}
		pos++;
		number = (long) number / 10;		//Shift {number} right 1 digit
	}
	
	num[pos] = 0x30 + number % 10;		//'0' + last digit of {number}
	for(;pos>=0;pos--) putch(num[pos]);	//Reverse the order of characters
}

// == putHex ==
// Print Hexadecimal (Unsigned)
void putHex(unsigned long number)
{
	char num[8];	//Max 8 Digits (32 Bits)
	int pos=0;
	while(number >> 4)
	{
		num[pos] = hexDigits[number&0xF];
		pos++;
		number >>= 4;		//Shift {number} right 1 digit
	}
	num[pos] = hexDigits[number&0xF];
	for(;pos>(0-1);pos--) putch(num[pos]);	//Reverse the order of characters
}

// == putOct ==
// Print Octal (Unsigned)
void putOct(unsigned long number)
{
	char num[11];	//Max 11 Digits (32Bit)
	int pos=0;
	while(number >> 3)
	{
		num[pos] = '0' + (number&07);
		pos++;
		number >>= 3;		//Shift {number} right 1 digit
	}
	num[pos] = hexDigits[number&07];
	for(pos++;pos--;) putch(num[pos]);	//Reverse the order of characters
}

// == putBin ==
// Print Binary (Unsigned)
void putBin(unsigned long number)
{
	char num[32];	//32 Bit
	int pos=0;
	while(number >> 1)
	{
		num[pos] = 0x30 + (number&1);	//'0' + last digit of {number}
		pos++;
		number = number >> 1;		//Shift {number} right 1 digit
	}
	num[pos] = 0x30 + (number&1);	//'0' + last digit of {number}
	if(strlen(num)&1) num[pos++] = '0';
	for(;pos>(0-1);pos--) putch(num[pos]);	//Reverse the order of characters
}


//DEBUG HACK
#define PORT 0x3F8
void init_serial() {
	outportb(PORT + 1, 0x00);    // Disable all interrupts
	outportb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outportb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outportb(PORT + 1, 0x00);    //                  (hi byte)
	outportb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outportb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outportb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void write_serial(char a) {
	while( (inportb(PORT + 5) & 0x20) == 0 );
	outportb(PORT,a);
}
