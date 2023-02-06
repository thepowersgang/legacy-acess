/*
AcessOS 0.1
System Calls
*/
#include <acess.h>
#include <syscalls.h>

#define	SYSCALLS_DEBUG	0
#define DEBUG	(SYSCALLS_DEBUG | ACESS_DEBUG)

//IMPOTRTS
extern void syscall_stub();
extern void	MM_Dump();

//PROTOTYPES
void syscall_install();
Uint32 syscall_handler(struct regs *r);
//Uint32 syscall_linuxhandler(struct regs *r);

int syscall_version	(Uint *err, Uint buffer, Uint flags);
int syscall_exit	(Uint *err);
int syscall_open	(Uint *err, Uint pathAddr, int flags, int mode);
int syscall_close	(Uint *err, int fp);
int syscall_read	(Uint *err, int fp, unsigned int length, Uint bufferAddr);
int syscall_write	(Uint *err, int fp, unsigned int length,  Uint strAddr);
int syscall_seek	(Uint *err, int fp, unsigned int dist, int dir);
int syscall_tell	(Uint *err, int fp);
int syscall_fstat	(Uint *err, int fp, Uint buf);
int syscall_readdir	(Uint *err, int fp, Uint nameAddr);
int syscall_ioctl	(Uint *err, int fp, int id, Uint dataAddr);
int syscall_kexec	(Uint *err, Uint32 cmd, Uint args, Uint envp);
int syscall_fork	(Uint *err);
int syscall_execve	(Uint *err, Uint32 cmd, Uint args, Uint envp);
int syscall_waitpid	(Uint *err, Uint pid, Uint action);
int syscall_brk		(Uint *err, Uint pos);
int syscall_ldbin	(Uint *err, Uint path, Uint entry);
int syscall_debug	(Uint *err, Uint fmt, Uint arg1, Uint arg2, Uint arg3, Uint arg4);
int syscall_yield	(Uint *err);

//CODE
void Syscall_Install()
{
	IDT_SetGate(0xAC, (Uint32)syscall_stub, 0x08, 0xEF);	//32bit Trap Gate (Instead of Interrupt) - User Callable
	//idt_set_gate(0xAC, (Uint32)syscall_stub, 0x08, 0x8E);
	//idt_set_gate(0x80, (Uint32)syscall_linuxstub, 0x08, 0x8E);
	LogF("[PROC] System Calls Installed\n");
}

Uint32 syscall_handler(struct regs *r)
{
	#if DEBUG
		LogF("syscall_handler: #%i\n", r->eax);
		LogF(" syscall_handler: EBX=%x,ECX=%x,EDX=%x\n", r->ebx, r->ecx, r->edx);
	#endif
	
	switch(r->eax)
	{
	case SYS_NULL:		return syscall_version	(&r->ebx, r->ebx, r->ecx);	// SYS_VERSION
	case SYS_EXIT:		return syscall_exit	(&r->ebx);
	case SYS_OPEN:		return syscall_open	(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_CLOSE:		return syscall_close(&r->ebx, r->ebx);
	case SYS_READ:		return syscall_read	(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_WRITE:		return syscall_write(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_SEEK:		return syscall_seek	(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_TELL:		return syscall_tell	(&r->ebx, r->ebx);
	case SYS_FSTAT:		return syscall_fstat(&r->ebx, r->ebx, r->ecx);
		
	case SYS_READDIR:	return syscall_readdir	(&r->ebx, r->ebx, r->ecx);
	case SYS_IOCTL:		return syscall_ioctl	(&r->ebx, r->ebx, r->ecx, r->edx);
		
	case SYS_KEXEC:		return syscall_kexec	(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_EXECVE:	return syscall_execve	(&r->ebx, r->ebx, r->ecx, r->edx);
	case SYS_FORK:		return syscall_fork		(&r->ebx);
	case SYS_WAITPID:	return syscall_waitpid	(&r->ebx, r->ebx, r->ecx);
	case SYS_YIELD:		return syscall_yield	(&r->ebx);
		
	case SYS_BRK:		return syscall_brk	(&r->ebx, r->ebx);
	case SYS_LDBIN:		return syscall_ldbin(&r->ebx, r->ebx, r->ecx);
	
	case SYS_DEBUG:		return syscall_debug(&r->ebx, r->ebx, r->ecx, r->edx, r->edi, r->esi);
	}
	
	LogF("syscall_handler: RETURN 0 (Bas Syscall %i)\n", r->eax);
	
	return 0;
}

/**
 \fn int syscall_version (Uint *err, char *buffer);
 \brief Returns the Acess Version String
 \param err	Pointer to EBX for Error Code
 \param buffer	Destination buffer for version string
 \param flags	String flags - unused ATM
 \todo	Implement Completely
*/
#define	ACESS_VER_STR	"AcessOS v1"
int syscall_version(Uint *err, Uint buffer, Uint flags)
{
	if(buffer == 0)	return sizeof(ACESS_VER_STR);
	strcpy((char*)buffer, ACESS_VER_STR);
	return 0;
}

/**
 \fn int syscall_exit(Uint *err)
 \brief Quits the current process
 \param err		Pointer to EBX
*/
int	syscall_exit(Uint *err)
{
	#if DEBUG
		LogF("syscall_exit: ()\n");
	#endif
	Proc_Exit();	// Exit Process
	for(;;)	Proc_Yield();	// Inifinitely Yield until process is destroyed
	return 1;
}

/**
 \fn int syscall_open(Uint *err, Uint pathAddr, int flags, int mode)
 \brief Opens a file with the VFS
 \param err		Pointer to EBX (errno)
 \param pathAddr	Address of the desired path
 \param flags	Open Flags
 \param	mode	Open Mode (Is this needed?)
*/
int syscall_open(Uint *err, Uint pathAddr, int flags, int mode)
{
	#if DEBUG
		LogF("syscall_open: (pathAddr=0x%x('%s'),flags=%i,mode=%i)\n", pathAddr,(char*)pathAddr,flags,mode);
	#endif
	return vfs_open((char *)pathAddr, flags|VFS_OPENFLAG_USER);
}

/**
 \fn int syscall_close(Uint *err, int fp)
 
*/
int syscall_close(Uint *err, int fp)
{
	vfs_close(fp);
	return 1;
}

int syscall_read(Uint *err, int fp, unsigned int length, Uint bufferAddr)
{
	return vfs_read(fp, length, (char *)bufferAddr);
}

int syscall_write(Uint *err, int fp, unsigned int length,  Uint strAddr)
{
	#if DEBUG >= 2
		LogF("syscall_write: (fp=%i,length=%i,strAddr=0x%x)\n", fp, length, strAddr);
	#endif
	return vfs_write(fp, length, (char *)strAddr);
}

int syscall_seek(Uint *err, int fp, unsigned int dist, int dir)
{
	#if DEBUG >= 2
		LogF("syscall_seek: (fp=%i,dist=%i,dir=0x%x)\n", fp, dist, dir);
	#endif
	vfs_seek(fp, dist, dir);
	return 1;
}

int syscall_tell(Uint *err, int fp)
{
	#if DEBUG
		LogF("syscall_tell: (fp=%i)\n", fp);
	#endif
	return vfs_tell(fp);
}

int syscall_readdir(Uint *err, int fp, Uint nameAddr)
{
	#if DEBUG
		LogF("syscall_readdir: (fp=%i, nameAddr=0x%x)\n", fp, nameAddr);
	#endif
	return vfs_readdir(fp, (char *)nameAddr);
}

int syscall_fstat(Uint *err, int fp, Uint buf)
{
	#if DEBUG
		LogF("syscall_fstat: (fp=%i, buf=0x%x)\n", fp, buf);
	#endif
	if(vfs_stat(fp, (t_fstat*)buf) == 0) {
		return -1;
	}
	return 1;
}


int syscall_ioctl(Uint *err, int fp, int id, Uint dataAddr)
{
	return vfs_ioctl(fp, id, (void*)dataAddr);
}

int syscall_execve(Uint *err, Uint32 cmd, Uint argp, Uint envp)
{
	Proc_Execve((char*)cmd, (char**)argp, (char**)envp, 3);
	return 0;
}

int syscall_fork(Uint *err)
{
	#if DEBUG
	int ret = Proc_Fork();
	__asm__ ("cli");
	MM_Dump();
	__asm__ ("sti");
	return ret;
	#else
	return Proc_Fork();
	#endif
}

int syscall_kexec(Uint *err, Uint32 cmd, Uint argp, Uint envp)
{
	return Proc_Open((char*)cmd, (char**)argp, (char**)envp);
}

int syscall_waitpid(Uint *err, Uint pid, Uint action)
{
	return Proc_WaitPid(pid, action);
}

int syscall_brk(Uint *err, Uint pos)
{
	//#if DEBUG
	Uint ret;
	ret = proc_brk(pos);
	LogF("syscall_brk: RETURN 0x%x\n", ret);
	return ret;
	//#else
	//return proc_brk(pos);
	//#endif
}

/**
 \fn int syscall_yield(Uint *err)
*/
int syscall_yield(Uint *err)
{
	Proc_Yield();
	return 0;
}

/**
 \fn int syscall_ldbin(Uint *err, Uint path, Uint entry)
 \brief Loads a binary file into the current process space and returns it's base
 \param	err		Pointer to EBX (errno)
 \param	path	Binary Path (String Pointer)
 \param	entry	Pointer to entrypoint
 \return	Base address of binary
*/
int syscall_ldbin(Uint *err, Uint path, Uint entry)
{
	return Binary_Load((char*)path, (Uint*)entry);
}

/**
 \fn int syscall_debug(Uint *err, Uint fmt, Uint arg1, Uint arg2, Uint arg3, Uint arg4)
 \brief Allows a user application to easily print debug text
*/
int syscall_debug(Uint *err, Uint fmt, Uint arg1, Uint arg2, Uint arg3, Uint arg4)
{
	#if !PRODUCTION
	#if DEBUG
	LogF("syscall_debug: (fmt=0x%x, arg1=0x%x, arg2=0x%x, arg3=0x%x, arg4=0x%x)\n",
		fmt, arg1, arg2, arg3, arg4);
	#endif
	LogF("[UDBG] %i: ", proc_pid);
	LogF((char*)fmt, arg1, arg2, arg3, arg4);
	#endif
	return 0;
}
