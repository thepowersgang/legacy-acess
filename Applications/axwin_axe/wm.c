/*
Acess OS GUI
Window Manager
*/
#include "header.h"

#define	DEBUG	0
#define EXPORT	

// === STRUCTURES ===
typedef struct sWINDOW_LIST {
	struct sWINDOW_LIST	*next;
	tWINDOW	*wnd;
}	tWINDOW_LIST;

// === IMPORTS ===
extern int	giScreenWidth,giScreenHeight;

//GLOBALS
static tWINDOW		*hwndRootWindow = NULL;
static tWINDOW_LIST	*windowList = NULL;

//PROTOTYPES
static void wmDrawWindow(tWINDOW *wnd);
static void wmRepaintWindow(tWINDOW *wnd);
static int	wmRepaintScreen(tWINDOW_LIST *ent);
static void wmInvalidateWindow(tWINDOW	*wnd);

//CODE
int	wmUpdateWindows()
{
	if(windowList == NULL)	return 0;
	return wmRepaintScreen(windowList);
}

EXPORT void	*WM_CreateWindow(int x, int y, int w, int h, wndproc_t wndProc, Uint flags)
{
	tWINDOW	*tmpWindow;
	tWINDOW_LIST	*tmpListEnt;
	
	if(x < 0 || y < 0 || wndProc == NULL)
		return 0;
	
	tmpWindow = (tWINDOW*)malloc(sizeof(tWINDOW));
	if(tmpWindow == NULL)
		return NULL;
	
	tmpWindow->handle = tmpWindow;
	tmpWindow->flags = flags;
	tmpWindow->wndproc = wndProc;
	tmpWindow->bmp.bpp = 32;
	tmpWindow->bmp.width = (w==-1?giScreenWidth:w);
	tmpWindow->bmp.height = (h==-1?giScreenHeight:h);
	tmpWindow->bmp.data = (void *) malloc(4*tmpWindow->bmp.width*tmpWindow->bmp.height);
	if(tmpWindow->bmp.data == NULL)
	{
		free(tmpWindow);
		return NULL;
	}
	tmpWindow->rc.x1 = x;
	tmpWindow->rc.y1 = y;
	tmpWindow->rc.x2 = x + tmpWindow->bmp.width;
	tmpWindow->rc.y2 = y + tmpWindow->bmp.height;
	tmpWindow->next = NULL;
	tmpWindow->prev = NULL;
	tmpWindow->first_child = NULL;
	tmpWindow->last_child = NULL;
	tmpWindow->parent = NULL;
	
	if(flags & WNDFLAG_SHOW)
	{
		tmpListEnt = malloc(sizeof(tWINDOW_LIST));
		tmpListEnt->wnd = tmpWindow;
		tmpListEnt->next = windowList;
		windowList = tmpListEnt;
	}
	
	if(hwndRootWindow == NULL)
	{
		hwndRootWindow = tmpWindow;
	}
	else
	{
		if(hwndRootWindow->first_child == NULL)
		{
			hwndRootWindow->last_child = tmpWindow;
			hwndRootWindow->first_child = tmpWindow;
		}
		else
		{
			hwndRootWindow->last_child->next = tmpWindow;
			tmpWindow->prev = hwndRootWindow->last_child;
			tmpWindow->prev->next = tmpWindow;
			hwndRootWindow->last_child = tmpWindow;
		}
		tmpWindow->parent = hwndRootWindow;
	}
	
	wmInvalidateWindow(tmpWindow);
	
	return tmpWindow;
}

EXPORT int	WM_SendMessage(void *hwnd, int msg, int a1, int a2)
{
	if(hwnd == NULL || ((tWINDOW*)hwnd)->wndproc == NULL)
	{
		return -1;
	}
	return ((tWINDOW*)hwnd)->wndproc( ((tWINDOW*)hwnd)->handle, msg, a1, a2 );
}

static void wmRepaintWindow(tWINDOW *wnd)
{
	tWINDOW	*child;
	
	if(wnd->repaint == 1)
	{
		for(child = wnd->first_child; child != NULL; child = child->next) {
			wmRepaintWindow(child);
		}
		WM_SendMessage(wnd, WM_REPAINT, (Uint)&wnd->bmp, 0);
		wmDrawWindow(wnd);
		wnd->repaint = 0;
	}
}

static int wmRepaintScreen(tWINDOW_LIST *ent)
{
	int ret = 0;
	while(ent)
	{
		if(ent->wnd->repaint == 1)
		{
			WM_SendMessage(ent->wnd, WM_REPAINT, (Uint)&ent->wnd->bmp, 0);
			wmDrawWindow(ent->wnd);
			ret = 1;
		}
		ent = ent->next;
	}
	return ret;
}

static void wmDrawWindow(tWINDOW *wnd)
{
	if(wnd->flags & WNDFLAG_SHOW) {
		draw_bmp(&wnd->bmp, &wnd->rc);
	}
}

static void wmInvalidateWindow(tWINDOW	*wnd)
{
	tWINDOW	*parent;
	wnd->repaint = 1;
	
	while( (parent = wnd->parent) )
		parent->repaint = 1;
}
