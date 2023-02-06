/*
Acess OS Window Manager
*/
#include "header.h"
#include "bitmaps.h"

#define	USE_MOUSE	1

//GLOBALS
 int	giConsoleFP = -1;
 int	giScreenFP = -1;
 int	giScreenMode = 1;
 int	giScreenWidth = SCREEN_WIDTH;
 int	giScreenHeight = SCREEN_HEIGHT;
 int	giScreenDepth = 32;
 int	giScreenSize = SCREEN_WIDTH*SCREEN_HEIGHT*32/8;
Uint32	*gScreenBuffer;
 int	giMouseFP = -1;
 int	giMouseSensitivity = 2;
Uint32	gCursorBMP[sizeof(bmpCursor)*2] = {0};
 int	mouseX=0, mouseY=0;
Uint8	buttonState;
Uint32	gCursorUnder[sizeof(bmpCursor)*2] = {0};
 int	giOldMouseX=-1, giOldMouseY=-1;

//PROTOTYPES
extern void Desktop_Init();
extern void heap_init();
void UpdateScreen();
#if USE_MOUSE
void UpdateMouse();
void BuildCursor();
void DrawCursor();
#endif
void memsetd(void *to, long val, int count);

//CODE
int main(void)
{
	struct {
		short	id;
		short	width, height;
		short	bpp;
	}	screenInfo = {0, SCREEN_WIDTH, SCREEN_HEIGHT, 32};
	
	giConsoleFP = open("/Devices/vterm/1", OPEN_FLAG_WRITE, 0);
	//giConsoleFP = 0;	// Terminal is File #0
	
	write(giConsoleFP, 22, "AcessOS GUI version 1\n");
	write(giConsoleFP, 43, "Opening and Initialising Video and Mouse...");
	
	giScreenFP = open("/Devices/vesa", OPEN_FLAG_WRITE, 0);
	#if USE_MOUSE
	giMouseFP = open("/Devices/ps2mouse", OPEN_FLAG_READ, 0);
	#endif
	
	// Get Screen Mode
	giScreenMode = ioctl(giScreenFP, 3, &screenInfo);	//Get Screen Mode
	giScreenWidth = screenInfo.width;
	giScreenHeight = screenInfo.height;
	giScreenDepth = screenInfo.bpp;
	
	// Allocate Screen Buffer
	giScreenSize = giScreenWidth*giScreenHeight*giScreenDepth/8;
	gScreenBuffer = malloc(giScreenSize);
	if(gScreenBuffer == NULL) {
		write(giConsoleFP, 49, "Unable to allocate double buffer (gScreenBuffer)\n");
		return 0;
	}
	
	// Set Screen Mode
	ioctl(giScreenFP, 1, &giScreenMode);
	
	#if USE_MOUSE
	ioctl(giMouseFP, 2, &giScreenWidth);	//Set Max X
	ioctl(giMouseFP, 3, &giScreenHeight);	//Set Max Y
	ioctl(giMouseFP, 1, &giMouseSensitivity);	//Set Max Y
	#endif
	write(giConsoleFP, 6, "Done.\n");
	
	UpdateScreen();
	Desktop_Init();
	#if USE_MOUSE
	BuildCursor();	//Create Cursor
	#endif
	
	memsetd(gScreenBuffer, 0x6666FF, giScreenSize/4);	// Set Background Colour
	UpdateScreen();
	for(;;) {
		memsetd(gScreenBuffer, 0x6666FF, giScreenSize/4);	// Set Background Colour
		//if(wmUpdateWindows())
		//	UpdateScreen();
		#if USE_MOUSE
		UpdateMouse();
		DrawCursor();
		#endif
	}
	
	return 1;
}

/* Copy from the buffer to the screen
 */
void UpdateScreen()
{
	//write(giConsoleFP, 22, "Updating Framebuffer.\n");
	seek(giScreenFP, 0, 1);	//SEEK SET
	write(giScreenFP, giScreenSize, gScreenBuffer);
}

#if USE_MOUSE
void UpdateMouse()
{
	struct {
		int	x, y, scroll;
		Uint8	buttons;
	} data;
	//k_printf("Updating Mouse State...");
	
	seek(giMouseFP, 0, 1);
	read(giMouseFP, sizeof(data), &data);
	
	mouseX = data.x;	mouseY = data.y;
	//Button Press
	if(data.buttons & ~buttonState) {
		//wmMessageButtonDown();
	}
	//Button Release
	if(~data.buttons & buttonState) {
		//wmMessageButtonUp();
	}
	
	buttonState = data.buttons;	//Update Button State
	//k_printf("Done.\n");
}

void DrawCursor()
{
	int i,j;
	Uint32	*bi;
	Uint32	buf[16];
	bi = gCursorBMP;
	//buf = gScreenBuffer + mouseY*giScreenWidth + mouseX;
	
	// Fast Return
	if(giOldMouseX == mouseX && giOldMouseY == mouseY)	return;
	
	// Restore beneath cursor
	if(giOldMouseX > -1)
	{
		seek(giScreenFP, (giOldMouseY*giScreenWidth + giOldMouseX)*4, 1);	//SEEK SET
		for(i=0;i<24;i++)
		{
			write(giScreenFP, 16*4, gCursorUnder+i*16);
			seek(giScreenFP, giScreenWidth*4-16*4, 0);	//SEEK CUR
		}
	}
	
	// Seek to new position
	seek(giScreenFP, (mouseY*giScreenWidth + mouseX)*4, 1);	//SEEK SET
	
	// Print Cursor
	for(i=0;i<24;i++) {
		for(j=0;j<16;j++) {
			gCursorUnder[i*16+j] = gScreenBuffer[ (mouseY+i)*giScreenWidth + mouseX + j ];
			if(*bi&0xFF000000)
				buf[j] = *bi&0xFFFFFF;
			else
				buf[j] = gCursorUnder[i*16+j];
			bi++;
		}
		write(giScreenFP, 16*4, buf);
		seek(giScreenFP, giScreenWidth*4-16*4, 0);	//SEEK CUR
	}
	
	// Set old position
	giOldMouseY = mouseY;
	giOldMouseX = mouseX;
}

void BuildCursor()
{
	int i,j;
	Uint32	px;
	Uint32	*buf, *bi;
	bi = bmpCursor;
	buf = gCursorBMP;
	for(i=0;i<sizeof(bmpCursor)/8;i++)
	{
		for(j=0;j<8;j++)
		{
			px = (*bi & (0xF << ((7-j)*4) )) >> ((7-j)*4);
			if( px & 0x1 )
			{
				if(px & 8)	*buf = 0xFF000000;	//Black (100% Alpha)
				else		*buf = 0xFFFFFFFF;	//White (100% Alpha)
			}
			else
				*buf = 0;	// 0% Alpha
			buf++;
		}
		bi++;
		for(j=0;j<8;j++)
		{
			px = (*bi & (0xF << ((7-j)*4) )) >> ((7-j)*4);
			if( px & 0x1 )
			{
				if(px & 8)	*buf = 0xFF000000;	// Black (100% Alpha)
				else		*buf = 0xFFFFFFFF;	// White (100% Alpha)
			}
			else
			{
				*buf = 0; // 0% Alpha
			}
			buf++;
		}
		bi++;
	}
}
#endif

#define USE_REP	1
void memcpyd(void *to, void *from, int count)
{
	#if USE_REP
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"movsl;\n\t"
		"popf;\n\t"
		: : "c" (count), "S" (from), "D" (to) );
	#else
	Uint32	*t = to;
	Uint32	*f = from;
	for(;count;count--)
		*t++ = *f++;
	#endif
}
void memcpy(void *to, void *from, unsigned int count)
{
	#if USE_REP
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"movsb;\n\t"
		"popf;\n\t"
		: : "c" (count), "S" (from), "D" (to) );
	#else
	Uint32	*t = to;
	Uint32	*f = from;
	for(;count;count--)
		*t++ = *f++;
	#endif
}

void memsetd(void *to, long val, int count)
{
	#if USE_REP
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"stosl;\n\t"
		"popf;\n\t"
		: : "c" (count), "a" (val), "D" (to) );
	#else
	Uint32	*t = to;
	for(;count;count--)
		*t++ = val;
	#endif
}
