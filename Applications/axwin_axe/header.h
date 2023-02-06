/*
Acess OS GUI
*/
#ifndef	_AXWIN_HEADER_H
#define _AXWIN_HEADER_H

#include <stdlib.h>
#include "../axe.h"


//CONSTANTS
#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define	SCREEN_PX_COUNT	(SCREEN_WIDTH*SCREEN_HEIGHT)
#define	SCREEN_BUFFER_SIZE	(SCREEN_WIDTH*SCREEN_HEIGHT*4)

#define NULL	((void*)0)

#include "axwin.h"

extern Uint32	*gScreenBuffer;

//PROTOTYPES
extern int	wmUpdateWindows();
extern void memcpyd(void *to, void *from, int count);
extern void draw_line(int x1, int y1, int x2, int y2, Uint32 colour);
extern void draw_rect(int x, int y, int w, int h, Uint32 colour);
extern void draw_bmp(BITMAP *bmp, RECT *rc);

#endif
