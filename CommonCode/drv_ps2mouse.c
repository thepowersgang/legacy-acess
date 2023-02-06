/*
AcessOS 0.1 Basic
PS/2 Mouse Driver
*/
#include <acess.h>
#include <vfs.h>
#include <fs_devfs2.h>

#define PS2MOUSE_DEBUG	0
#define DEBUG	(PS2MOUSE_DEBUG | ACESS_DEBUG)

//TYPEDEFS & STRUCTS
typedef struct {
	int	x, y;
	int	scroll;
	Uint8	buttons;
} t_mouse_fsdata;

// == CONSTANTS ==
#define PS2_IO_PORT	0x60
#define MOUSE_BUFFER_SIZE	(sizeof(t_mouse_fsdata))
#define MOUSE_SENS_BASE	5
enum {
	MOUSE_IOCTL_NULL,
	MOUSE_IOCTL_SENSITIVITY,
	MOUSE_IOCTL_MAX_X,
	MOUSE_IOCTL_MAX_Y
};

// == GLOBALS ==
devfs_driver	gMouse_DriverStruct = {{0}, 0};
int	giMouse_DriverId = -1;
int	giMouse_Sensitivity = 1;
t_mouse_fsdata	gMouse_Data = {0, 0, 0, 0};
int giMouse_MaxX = 640, giMouse_MaxY = 480;
int	giMouse_Cycle = 0;	// IRQ Position
Uint8	gaMouse_Bytes[4] = {0,0,0,0};

// == PROTOTYPES ==
// - Internal -
void PS2Mouse_Irq(struct regs *r);
static void mouseSendCommand(Uint8 cmd);
static void enableMouse();
// - Filesystem -
static int	PS2Mouse_read(vfs_node *node, int off, int len, void *buffer);
static int	PS2Mouse_ioctl(vfs_node *node, int id, void *data);

// == CODE ==
int PS2Mouse_Install()
{
	// Make Root Node
	gMouse_DriverStruct.rootNode.name = "ps2mouse";
	gMouse_DriverStruct.rootNode.nameLength = 8;
	gMouse_DriverStruct.rootNode.uid = 0;	gMouse_DriverStruct.rootNode.gid = 0;
	gMouse_DriverStruct.rootNode.mode = 0444;	gMouse_DriverStruct.rootNode.flags = 0;
	// /*
	gMouse_DriverStruct.rootNode.ctime = 
		gMouse_DriverStruct.rootNode.mtime = 
		gMouse_DriverStruct.rootNode.atime = now();
	gMouse_DriverStruct.rootNode.readdir = NULL;	gMouse_DriverStruct.rootNode.finddir = NULL;
	gMouse_DriverStruct.rootNode.read = PS2Mouse_read;	gMouse_DriverStruct.rootNode.write = NULL;
	gMouse_DriverStruct.rootNode.close = NULL;
	
	// Make Driver Structure
	gMouse_DriverStruct.ioctl = PS2Mouse_ioctl;
	
	giMouse_DriverId = dev_addDevice(&gMouse_DriverStruct);
	if(giMouse_DriverId == -1)
		return 0;
	gMouse_DriverStruct.rootNode.impl = giMouse_DriverId;	//Used by DevFS
	
	// Initialise Mouse Controller
	irq_install_handler(12, PS2Mouse_Irq);	// Set IRQ
	giMouse_Cycle = 0;	// Set Current Cycle position
	enableMouse();		// Enable the mouse
	
	return 1;
}

/* Handle Mouse Interrupt
 */
void PS2Mouse_Irq(struct regs *r)
{
	Uint8	flags;
	int	dx, dy;
	//int	log2x, log2y;
	
	gaMouse_Bytes[giMouse_Cycle] = inportb(0x60);
	if(giMouse_Cycle == 0 && !(gaMouse_Bytes[giMouse_Cycle] & 0x8))
		return;
	giMouse_Cycle++;
	if(giMouse_Cycle == 3)
		giMouse_Cycle = 0;
	else
		return;
	
	if(giMouse_Cycle > 0)
		return;
	
	// Read Flags
	flags = gaMouse_Bytes[0];
	if(flags & 0xC0)	// X/Y Overflow
		return;
		
	#if DEBUG
	//LogF(" PS2Mouse_Irq: flags = 0x%x\n", flags);
	#endif
	
	// Calculate dX and dY
	dx = (int) ( gaMouse_Bytes[1] | (flags & 0x10 ? ~0x00FF : 0) );
	dy = (int) ( gaMouse_Bytes[2] | (flags & 0x20 ? ~0x00FF : 0) );
	dy = -dy;	// Y is negated
	#if DEBUG
	LogF(" PS2Mouse_Irq: RAW dx=%i, dy=%i\n", dx, dy);
	#endif
	dx = dx*MOUSE_SENS_BASE/giMouse_Sensitivity;
	dy = dy*MOUSE_SENS_BASE/giMouse_Sensitivity;
	
	//__asm__ __volatile__ ("bsr %%eax, %%ecx" : "=c" (log2x) : "a" ((Uint)gaMouse_Bytes[1]));
	//__asm__ __volatile__ ("bsr %%eax, %%ecx" : "=c" (log2y) : "a" ((Uint)gaMouse_Bytes[2]));
	//LogF(" PS2Mouse_Irq: dx=%i, log2x = %i\n", dx, log2x);
	//LogF(" PS2Mouse_Irq: dy=%i, log2y = %i\n", dy, log2y);
	//dx *= log2x;
	//dy *= log2y;
	
	// Set Buttons
	gMouse_Data.buttons = flags & 0x7;	//Buttons (3 only)
	
	// Update X and Y Positions
	gMouse_Data.x += (Sint)dx;
	gMouse_Data.y += (Sint)dy;
	
	// Constrain Positions
	if(gMouse_Data.x < 0)	gMouse_Data.x = 0;
	if(gMouse_Data.y < 0)	gMouse_Data.y = 0;
	if(gMouse_Data.x >= giMouse_MaxX)	gMouse_Data.x = giMouse_MaxX-1;
	if(gMouse_Data.y >= giMouse_MaxY)	gMouse_Data.y = giMouse_MaxY-1;	
}

/* Read mouse state (coordinates)
 */
static int PS2Mouse_read(vfs_node *node, int off, int len, void *buffer)
{
	if(len < MOUSE_BUFFER_SIZE)
		return -1;

	memcpy(buffer, &gMouse_Data, MOUSE_BUFFER_SIZE);
		
	return len;
}

/* Handle messages to the device
 */
static int PS2Mouse_ioctl(vfs_node *node, int id, void *data)
{
	//printf("mouse_ioctl: (id=%i, (int)data=%i)\n", id, intData);
	
	switch(id)
	{
	
	case MOUSE_IOCTL_NULL:
		return -2;
		
	case MOUSE_IOCTL_SENSITIVITY:
		if(data != NULL) {
			giMouse_Sensitivity = *(int*)data;
		}
		return giMouse_Sensitivity;
		break;
		
	case MOUSE_IOCTL_MAX_X:
		if(data != NULL) {
			giMouse_MaxX = *(int*)data;
		}
		return giMouse_MaxX;
		break;
		
	case MOUSE_IOCTL_MAX_Y:
		if(data != NULL) {
			giMouse_MaxY = *(int*)data;
		}
		return giMouse_MaxY;
		break;
	
	default:
		return -2;
		break;
	
	}
}

//== Internal Functions ==
static inline void mouseOut64(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inportb(0x64) & 2 );	// Wait for Flag to clear
	outportb(0x64, data);	// Send Command
}
static inline void mouseOut60(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inportb(0x64) & 2 );	// Wait for Flag to clear
	outportb(0x60, data);	// Send Command
}
static inline Uint8 mouseIn60()
{
	int timeout=100000;
	while( timeout-- && (inportb(0x64) & 1) == 0);	// Wait for Flag to set
	return inportb(0x60);
}
static void mouseSendCommand(Uint8 cmd)
{
	mouseOut64(0xD4);
	mouseOut60(cmd);
}

extern volatile int	kb_lastChar;

static void enableMouse()
{
	Uint8	status;
	printf("[P/S2] Enabling Mouse...");
	
	// Enable AUX PS/2
	mouseOut64(0xA8);
	
	// Enable AUX PS/2 (Compaq Status Byte)
	mouseOut64(0x20);	// Send Command
	status = mouseIn60();	// Get Status
	status &= 0xDF;	status |= 0x02;	// Alter Flags (Set IRQ12 (2) and Clear Disable Mouse Clock (20))
	mouseOut64(0x60);	// Send Command
	mouseOut60(status);	// Set Status
	
	//mouseSendCommand(0xF6);	// Set Default Settings
	mouseSendCommand(0xF4);	// Enable Packets
	printf("Done\n");
}
