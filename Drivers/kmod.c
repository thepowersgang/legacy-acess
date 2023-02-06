/*
AcessOS Version 1
Kernel Module Linking
*/
//Get Required structures
#include "kmod.h"

//=== DEFINES ===
#define	MAGIC_VALUE		0xACE55051
#define	ACESSBASIC_EXPORT	0x00100020
#define	ACESSOS_EXPORT		0xC0105000

//=== STRUCTURES ===
typedef struct {
	 int	(*addDevice)(devfs_driver *drv);
	void	(*delDevice)(int drv);
	 int	(*addSymlink)(int drv, int handle, char *name);
	void	(*delSymlink)(int id);
} __IMPORT_devfs_t;

typedef struct {
	 int	(*addfs)(Uint16 ident, vfs_node* (*initDev)(char* device));
	 int	(*open)(char *path, int flags);
	void	(*close)(int fileHandle);
	 int	(*read)(int fileHandle, int length, void *buffer);
	 int	(*write)(int fileHandle, int length, void *buffer);
	void	(*seek)(int fileHandle, int distance, int flag);
	 int	(*tell)(int fileHandle);
	 int	(*readdir)(int fileHandle, char *name);
	 int	(*mount)(char *device, char *mountPoint, Uint8 filesystem);
	 
	 int	(*inode_initCache)(void);
	void	(*inode_clearCache)(int handle);
	vfs_node*	(*inode_getCache)(int handle, Uint32 inode);
	vfs_node*	(*inode_cacheNode)(int handle, Uint32 inode, vfs_node *node);
	void	(*inode_uncacheNode)(int handle, Uint32 inode);
} __IMPORT_vfs_t;

typedef struct {
	 int	(*now)();
	void	(*time_wait)(int ticks);
	 int	(*timestamp)(int sec, int mins, int hrs, int day, int month, int year);
	void*	(*malloc)(Uint32 bytes);
	void	(*free)(void *mem);
	void*	(*realloc)(Uint32 bytes, void *oldPos);
	void	(*dma_setRW)(int channel, int read);
	 int	(*dma_readData)(int channel, int count, void *buffer);
	 int	(*time_createTimer)(float delay, void(*callback)(int), int arg, int oneshot);
	void	(*time_removeTimer)(int id);
	void	(*irq_install_handler)(int irq, void (*handler)(t_regs *r));
	void	(*irq_uninstall_handler)(int irq);
	void	(*LogF)(char *fmt, ...);
} __IMPORT_misc_t;

//=== GLOBALS ===
__IMPORT_vfs_t	*__IMPORT_vfs;
__IMPORT_devfs_t	*__IMPORT_devfs;
__IMPORT_misc_t	*__IMPORT_misc;

//=== CODE ===
/*
int __IMPORT_init()
 Initialises the import functions
*/
int __IMPORT_init()
{
	Uint32	*magic = (Uint32*) ACESSBASIC_EXPORT;
	if(*magic != MAGIC_VALUE) {
		magic = (Uint32*) ACESSOS_EXPORT;
		if(*magic != MAGIC_VALUE) {
			return 0;
		}
	}
	
	__IMPORT_vfs = (__IMPORT_vfs_t *) magic+1;
	__IMPORT_devfs = (__IMPORT_devfs_t *) magic+2;
	__IMPORT_misc = (__IMPORT_misc_t *) magic+4;
	return 1;
}

//--- Library Functions ---
// Locally Implemented
void memcpy(void *dest, const void *src, Uint32 count)
{
    const char *sp = (const char *)src;
    char *dp = (char *)dest;
    for(;count--;) *dp++ = *sp++;
}
void memcpyd(void *dst, void *src, Uint32 dwCount)
{
	for(;dwCount--;) {
		*((Uint32*)dst) = *((Uint32*)src);
		dst += 4;	src += 4;
	}
}
void *memset(void *dest, char val, Uint16 count)
{
    char *temp = (char *)dest;
    for(;count--;) *temp++ = val;
    return dest;
}
void *memsetw(void *dest, Uint16 val, Uint16 count)
{
    Uint16 *temp = (Uint16 *)dest;
    for(;count--;) *temp++ = val;
    return dest;
}
int strlen(const char *str)
{
    Uint16 retval;
    for(retval = 0; *str != '\0'; str++)
        retval++;
    return retval;
}
int strcmp(const char *str1, const char *str2)
{
	while(*str1 == *str2 && *str1 != '\0') {
		str1++; str2++;
	}
	return (int)*str1 - (int)*str2;
}
int strncmp(const char *str1, const char *str2)
{
	int cmp1, cmp2;
	cmp1 = *str1;	cmp2 = *str2;
	if('a' <= cmp1 && cmp1 <= 'z')	cmp1 -= 0x20;
	if('a' <= cmp2 && cmp2 <= 'z')	cmp2 -= 0x20;
	while(cmp1 == cmp2 && *str1 != '\0') {
		str1++; str2++;
		cmp1 = *str1;	cmp2 = *str2;
		if('a' <= cmp1 && cmp1 <= 'z')	cmp1 -= 0x20;
		if('a' <= cmp2 && cmp2 <= 'z')	cmp2 -= 0x20;
	}
	return cmp1 - cmp2;
}
void strcpy(char *dest, const char *src)
{
	while(src) {
		*dest = *src;
		src++; dest++;
	}
}
Uint8 inportb( Uint16 _port )
{
    Uint8 rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}
Uint16 inportw( Uint16 _port )
{
    Uint16 rv;
    __asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}
Uint32 inportd( Uint16 _port )
{
    unsigned long rv;
    __asm__ __volatile__ ("inl %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}
void outportb( Uint16 _port, Uint8 _data )
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}
void outportw( Uint16 _port, Uint16 _data )
{
    __asm__ __volatile__ ("outw %1, %0" : : "dN" (_port), "a" (_data));
}
void outportd( Uint16 _port, Uint32 _data )
{
	__asm__ __volatile__ ("outl %1, %0" : : "dN" (_port), "a" (_data));
}


//--- DevFS Functions ---
int	DevFS_AddDevice(devfs_driver *drv)
{
	return __IMPORT_devfs->addDevice(drv);
}
void DevFS_DelDevice(int drv)
{
	__IMPORT_devfs->delDevice(drv);
}
int DevFS_AddSymlink(int drv, int handle, char *name)
{
	return __IMPORT_devfs->addSymlink(drv, handle, name);
}
void DevFS_DelSymlink(int id)
{
	__IMPORT_devfs->delSymlink(id);
}

//--- VFS Functions ---
int VFS_AddFS(Uint16 ident, vfs_node* (*initDev)(char *device))
{
	return __IMPORT_vfs->addfs(ident, initDev);
}
int VFS_Open(char *path, int flags)
{
	return __IMPORT_vfs->open(path, flags);
}
void VFS_Close(int fileHandle)
{
	__IMPORT_vfs->close(fileHandle);
}
int	VFS_Read(int fileHandle, int length, void *buffer)
{
	return __IMPORT_vfs->read(fileHandle, length, buffer);
}
int	VFS_Write(int fileHandle, int length, void *buffer)
{
	return __IMPORT_vfs->write(fileHandle, length, buffer);
}
void VFS_Seek(int fileHandle, int distance, int flag)
{
	__IMPORT_vfs->seek(fileHandle, distance, flag);
}
int	VFS_Tell(int fileHandle)
{
	return __IMPORT_vfs->tell(fileHandle);
}
int	VFS_ReadDir(int fileHandle, char *name)
{
	return __IMPORT_vfs->readdir(fileHandle, name);
}
int	VFS_Mount(char *device, char *mountPoint, Uint8 filesystem)
{
	return __IMPORT_vfs->mount(device, mountPoint, filesystem);
}

int	Inode_InitCache(void)
{
	return __IMPORT_vfs->inode_initCache();
}
void Inode_ClearCache(int handle)
{
	__IMPORT_vfs->inode_clearCache(handle);
}
vfs_node *Inode_GetCache(int handle, Uint32 inode)
{
	return __IMPORT_vfs->inode_getCache(handle, inode);
}
vfs_node *Inode_CacheNode(int handle, Uint32 inode, vfs_node *node)
{
	return __IMPORT_vfs->inode_cacheNode(handle, inode, node);
}
void Inode_UncacheNode(int handle, Uint32 inode)
{
	return __IMPORT_vfs->inode_uncacheNode(handle, inode);
}

//--- Misc Functions ---
int now()
{
	return __IMPORT_misc->now();
}
int timestamp(int sec, int mins, int hrs, int day, int month, int year)
{
	return __IMPORT_misc->timestamp(sec, mins, hrs, day, month, year);
}
void *malloc(int bytes)
{
	return __IMPORT_misc->malloc(bytes);
}
void free(void *mem)
{
	__IMPORT_misc->free(mem);
}
void *realloc(int bytes, void *mem)
{
	return __IMPORT_misc->realloc(bytes, mem);
}
void DMA_SetRW(int channel, int read)
{
	__IMPORT_misc->dma_setRW(channel, read);
}
int DMA_ReadData(int channel, int count, void *buffer)
{
	return __IMPORT_misc->dma_readData(channel, count, buffer);
}
int Time_CreateTimer(float delay, void(*callback)(int), int arg, int oneshot)
{
	return __IMPORT_misc->time_createTimer(delay, callback, arg, oneshot);
}
void Time_RemoveTimer(int id)
{
	__IMPORT_misc->time_removeTimer(id);
}
void IRQ_Set(int irq, void (*handler)(t_regs *r))
{
	__IMPORT_misc->irq_install_handler(irq, handler);
}
void IRQ_Clear(int irq)
{
	__IMPORT_misc->irq_uninstall_handler(irq);
}
void LogF(char *fmt, ...)
{
	__asm__ __volatile__ ("jmp %%eax" :: "a" (__IMPORT_misc->LogF));
}