/*
AcessOS 1.0
TTY Driver
*/
#include <acess.h>
#include <vfs.h>
#include <fs_devfs2.h>

//DEFINES
#define VTERM_COUNT	8
#define VTERM_LINECACHE	125	// 5 Screens worth
#define VTERM_SIZE	(80*VTERM_LINECACHE)
#define DEFAULT_ATTRIBS	0x0F	//White on Black

//STRUCTURES
typedef struct {
	int		pos;
	Uint16	data[VTERM_SIZE+80*25];
	char	flags;
	vfs_node	node;
} tty_vterm_t;

//IMPORTS
extern volatile int kb_lastChar;

const char vt100vgaColors[8] = {
	0, 4, 2, 6, 1, 5, 3, 7
};

//GLOBALS
int		tty_currentTerm = 0;
int		tty_driver_id = -1;
char	tty_strbuf[VTERM_COUNT*2];
tty_vterm_t	tty_vterm[VTERM_COUNT];
devfs_driver	gTTTY_Info = {
	{0}, 0
};

//PROTOTYPES
int tty_read(vfs_node *node, int off, int len, void *buffer);
int tty_write(vfs_node *node, int off, int len, void *buffer);
vfs_node *tty_readdir(vfs_node *dirNode, int pos);
vfs_node *tty_finddir(vfs_node *dirNode, char *file);
int tty_ioctl(vfs_node *node, int id, void *data);

int tty_parseEscape(char *buf, int handle);

//CODE
int tty_read(vfs_node *node, int off, int len, void *buffer)
{
	char *buf = (char *)buffer;
	int	handle = node->inode - 1;
	int	i;
	
	if(handle != tty_currentTerm)	//Current Terminal
		return -1;
	
	for(i=0;i<len;i++) {
		while(kb_lastChar == -1) Proc_Yield();
		buf[i] = kb_lastChar;
		kb_lastChar = -1;
	}
	return len;
}

int tty_write(vfs_node *node, int off, int len, void *buffer)
{
	char *buf = (char *)buffer;
	int	i = 0;
	int	handle = node->inode - 1;
	int	tmp;
	tty_vterm_t	*vt;
	
	if( VTERM_COUNT <= handle || handle < 0 ) {
		return 0;
	}
	
	vt = &tty_vterm[handle];
	
	for(i=0;i<len;i++)
	{
		tmp = 0;
		switch(buf[i])
		{
		case 0x00:	// Null - Ignore
			tmp = -555;	//Hack - Ignore this character
			break;
		case 0x08:	//Backspace
			vt->data[vt->pos] = 0x20 | (vt->flags<<8);
			vt->pos --;
			// Eliminate Tabs
			while( (vt->data[vt->pos] & 0xFF) == 0 && vt->pos > 0 )
			{
				vt->pos --;
			}
			vt->pos ++;
			break;
		case 0x09:	//Tab
			tmp = vt->pos;
			vt->pos = ((vt->pos + 8)&~7);
			memsetw((Uint16*)(vt->data + tmp), (0x0 | (vt->flags<<8)), vt->pos-tmp);
			break;
		case '\n':	//Newline
			tmp = vt->pos;
			vt->pos /= 80;
			vt->pos *= 80;
			vt->pos += 80;
			memsetw((Uint16*)(vt->data + tmp), (0x0 | (vt->flags<<8)), vt->pos-tmp);
			break;
			
		case 0x1B:	//Escape
			tmp = -555;	//Hack - Ignore Escape Sequences
			if(i+1 < len)
			{
				i += tty_parseEscape((char*)(buf+i+1), handle);
			}
			break;
			
		default:
			if(buf[i] >= ' ')
			{
				vt->data[vt->pos] = buf[i] | (vt->flags<<8);
				vt->pos++;
			}
			break;
		}
		
		if(tmp != -555 && handle == tty_currentTerm)
			putch(buf[i]);
		
		if(vt->pos >= VTERM_SIZE)
			vt->pos -= VTERM_SIZE;
	}
	
	return len;
}

/* Read Dir
 */
vfs_node *tty_readdir(vfs_node *dirNode, int pos)
{
	if(pos >= VTERM_COUNT) {
		return NULL;
	}
	
	return &tty_vterm[pos].node;
}

/* Find Dir
 */
vfs_node *tty_finddir(vfs_node *dirNode, char *filename)
{
	int pos;
	pos = filename[0] - '1';
	
	if(filename[1] != '\0' || pos >= VTERM_COUNT || 0 > pos)
	{
		return NULL;
	}
	
	return &tty_vterm[pos].node;
}

int tty_ioctl(vfs_node *node, int id, void *data)
{
	return 0;
}

int tty_install()
{
	int i;
	
	//Fill Root Node Data
	gTTTY_Info.rootNode.name = "vterm";
	gTTTY_Info.rootNode.nameLength = 5;
	gTTTY_Info.rootNode.inode = 0;	gTTTY_Info.rootNode.length = VTERM_COUNT;
	gTTTY_Info.rootNode.uid = 0;	gTTTY_Info.rootNode.gid = 0;	gTTTY_Info.rootNode.mode = 0771;
	gTTTY_Info.rootNode.flags = VFS_FFLAG_DIRECTORY;
	gTTTY_Info.rootNode.ctime = gTTTY_Info.rootNode.mtime = gTTTY_Info.rootNode.atime = now();
	gTTTY_Info.rootNode.unlink = NULL;
	gTTTY_Info.rootNode.readdir = tty_readdir;
	gTTTY_Info.rootNode.finddir = tty_finddir;

	//Fill Driver struct Data
	gTTTY_Info.ioctl = tty_ioctl;

	memsetda(tty_vterm, 0, VTERM_COUNT*sizeof(tty_vterm_t)/4);
	
	for(i=0;i<VTERM_COUNT;i++)
	{
		tty_strbuf[i*2] = '1'+i;	tty_strbuf[i*2+1] = '\0';
		tty_vterm[i].node.name = (char*)(tty_strbuf+i*2);	tty_vterm[i].node.nameLength = 1;
		tty_vterm[i].node.inode = 1+i;		tty_vterm[i].node.length = 0;
		tty_vterm[i].node.uid = 0;	tty_vterm[i].node.gid = 0;	tty_vterm[i].node.mode = 0666;
		tty_vterm[i].node.flags = 0;
		tty_vterm[i].node.ctime = 
			tty_vterm[i].node.mtime = 
			tty_vterm[i].node.atime = now();
		tty_vterm[i].node.read = tty_read;
		tty_vterm[i].node.write = tty_write;
		tty_vterm[i].node.close = NULL;
		tty_vterm[i].node.unlink = NULL;
		
		tty_vterm[i].flags = DEFAULT_ATTRIBS;
	}
	
	//Register Driver
	gTTTY_Info.rootNode.impl = tty_driver_id = dev_addDevice(&gTTTY_Info);
	if(tty_driver_id == -1) {
		panic("Unable to install TTY driver");
	}
	return 1;
}

int tty_parseEscape(char *buf, int handle)
{
	char	c;
	int		num = 0, j = 1;
	int		args[4] = {0,0,0,0};
	tty_vterm_t *vt = &tty_vterm[handle];
	
	switch(buf[0]) {
	//Large Code
	case '[':
		c = buf[j];
		do {
			while('0' <= c && c <= '9') {
				args[num] *= 10;
				args[num] += c-'0';
				c = buf[++j];
			}
			num++;
		} while(c == ';');
		
		if(c == '"') {
			c = buf[++j];
			while(c != '"')
				c = buf[++j];
		}
		
		if(	('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
			switch(c) {
			//Clear By Line
			case 'J':
				//Full Screen
				if(args[0] == 2) {
					memsetw(vt->data, 0x20|(vt->flags<<8), VTERM_SIZE);
					vt->pos = 0;
					if(handle == tty_currentTerm) {
						scrn_cls();
					}
				}
				break;
			case 'm':
				for(;num--;) {
					if( 0 <= args[num] && args[num] <= 8) {	//Other Flags
						switch(args[num]) {
						case 0:	vt->flags = DEFAULT_ATTRIBS;	break;	//Reset
						case 1:	vt->flags |= 0x8;	break;	//Bright
						case 2:	vt->flags &= ~0x8;	break;	//Dim
						case 5:	vt->flags |= 0x80;	break;	//Blinking
						}
					}
					else if(30 <= args[num] && args[num] <= 37)	//Foreground
						vt->flags = (vt->flags&0xF8) | (vt100vgaColors[args[num]-30]);
					else if(40 <= args[num] && args[num] <= 47)	//Background
						vt->flags = (vt->flags&0x8F) | (vt100vgaColors[args[num]-40]<<4);
				}
				if(handle == tty_currentTerm) {
					VGAText_SetAttrib(vt->flags);
				}
				break;
			}
		}
		break;
		
	default:
		break;
	}
	
	return j+1;
}
