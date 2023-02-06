/*
AcessOS 0.1
System Calls
HEADER
*/

enum AcessSysCalls {
	SYS_NULL,	//0
	SYS_EXIT,	//1
	SYS_OPEN,	//2
	SYS_CLOSE,	//3
	SYS_READ,	//4
	SYS_WRITE,	//5
	SYS_SEEK,	//6
	SYS_TELL,	//7
	SYS_UNLINK,	//8
	SYS_GETPID,	//9
	SYS_KILL,	//10
	SYS_FSTAT,	//11
	
	SYS_READDIR,//12
	SYS_IOCTL,	//13
	
	SYS_WAIT,	//14
	SYS_KEXEC,	//15
	SYS_FORK,	//16
	SYS_EXECVE,	//17
	SYS_WAITPID,	//18
	SYS_BRK,	//19
	SYS_LDBIN,	//20 - Load Binary
	SYS_YIELD,	//21 - Yield Timeslice
	SYS_LAST,
	SYS_DEBUG = 0x100,	// 256 - Fast User Debug
	SYS_URET,	// 257 - Return from user mode
	SYS_CLAST
};

enum LinuxSysCalls {
	LSYS_NULL,
	LSYS_EXIT
};

enum WindowsSysCalls {
	WSYS_NULL
};
