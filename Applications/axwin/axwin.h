/*
AxWin API
*/

#ifndef NULL
# define NULL	((void*)0)
#endif

#ifndef Uint8
typedef unsigned char	Uint8;
#endif
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef unsigned int	Uint;

typedef int (*wndproc_t)(void *handle, int message, int arg1, int arg2);

typedef struct {
	short	width, height;
	int		bpp;
	void	*data;
} BITMAP;
typedef struct {
	int x1, y1;
	int x2, y2;
} RECT;

typedef struct sWINDOW{
	void	*handle;
	RECT	rc;
	char	*title;
	char	*Text;
	 int	TextFlags;
	 int	flags;
	 int	repaint;
	
	BITMAP	bmp;
	
	struct sWINDOW	*next, *prev;
	struct sWINDOW	*first_child, *last_child;
	struct sWINDOW	*parent;
	
	wndproc_t	wndproc;
} tWINDOW;

//Flag Values
#define WNDFLAG_SHOW		0x0001
#define WNDFLAG_NOBORDER	0x0010
#define WNDFLAG_RESIZEABLE	0x0020

//Window Messages
enum MESSAGES {
	WM_NULL,
	WM_REPAINT,
	WM_GETTEXT,
	WM_SETTEXT,
	WM_SETTITLE,
	WM_GETTITLE
};

// === EXTERNAL FUNCTIONS ===
extern void*	WM_CreateWindow(int x, int y, int w, int h, wndproc_t wndProc, Uint flags);
extern int	WM_SendMessage(void *hwnd, int msg, int a1, int a2);

// === DEFINES ===
#define WM_SetText(hwnd, text)	WM_SendMessage((hwnd), WM_SETTEXT, (int)((char*)(text)), 0)
#define WM_SetTitle(hwnd, text)	WM_SendMessage((hwnd), WM_SETTITLE, (int)((char*)(text)), 0)
