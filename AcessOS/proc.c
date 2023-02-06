/*
AcessOS v0.1
Process Sceduler/Controller
*/
#include <system.h>
#include <proc.h>
#include <proc_int.h>

#define PROC_DEBUG	1
#define DEBUG	(PROC_DEBUG | ACESS_DEBUG)

#define	STACK_BTM	0xA0000000
#define	STACK_TOP	0xB0000000
#define STACK_SIZE	10
#define STACK_COUNT	((STACK_TOP-STACK_BTM)/(STACK_SIZE<<12))
#define	ARG_HOME	0x200000	// 2 Mb
#define ARG_MAX		0x100000
#define	MAGIC_SWITCH	0x1337ACE9
#define	MAGIC_SWITCH_S	"$0x1337ACE9"
#define PROC_DEF_PRI	5

#define	SCHED_SANITY_CHECK	0

#define T_PROC_NEXT_OFS	((int)&(((t_proc*)NULL)->next))

//IMPORTS
extern int	sys_stack;
extern void	Proc_Stack_Top;
extern Uint	Binary_Load(char *file, Uint *entryPoint);
extern void	sched();
extern Uint	getEip();
extern Uint	getEipSafe(Uint *esp);
extern void Proc_DropToUsermode();
extern void Proc_ProcessStart(Uint *stack);
extern void User_ReturnExit();		// Usermode Return

extern void time_handler(struct regs *r);

//GLOBALS
t_proc	processes[PROC_MAX_PID];
t_proc_tss	proc_tss[MAX_CPUS];
 int	proc_pid = 0;
 int	proc_nextPid = 1;
t_proc	*gpCurrentProc = NULL;
t_proc	*gpProcsActive = NULL;
t_proc	*gpProcsAsleep = NULL;
t_procWait	*gpProcsWaiting = NULL;

//PROTOTYPES
void	tss_setup();
void	Proc_Exit();
 int	Proc_Kill(int pid);
void	Proc_DumpRunning();
Uint	Proc_MakeUserStack();

//CODE
/**
 \fn void Proc_Install()
 \brief Installs process handler
 
 Installs scheduler to IRQ0 (timer)
 and halts system waiting for interrupt.
*/
void Proc_Install()
{
	gpCurrentProc = malloc(sizeof(t_proc));
	gpCurrentProc->status = PROCSTAT_ACTIVE;
	gpCurrentProc->pid = 0;
	gpCurrentProc->ppid = 0;
	gpCurrentProc->slots =
		gpCurrentProc->timeRem = PROC_DEF_PRI;
	gpCurrentProc->endBss = 0;
	gpCurrentProc->next = NULL;
	gpCurrentProc->esp =
		gpCurrentProc->ebp =
		gpCurrentProc->eip = 0;
	gpCurrentProc->uid = 0;	// Root
	gpCurrentProc->gid = 0;
	gpCurrentProc->ImageName = "KERNEL";
	gpProcsActive = gpCurrentProc;
	// Flush TSS
	__asm__ __volatile__ ("ltr %%ax": : "a" ((5<<3)/*|3*/));
	// Set Scheduler Handler
    IDT_SetGate(IRQ_OFFSET+0, (unsigned)sched, 0x08, 0x8E);
	LogF("[PROC] Scheduler Started\n");
}

/*
int Proc_Fork()
- Creates a process that is a complete clone of the
  current process
*/
int Proc_Fork()
{
	 int	pid;
	Uint	eip;
	t_proc	*proc, *tmpproc;
	
	//#if DEBUG
	//	LogF("Proc_Fork: ()\n");
	//#endif
	
	// = Clear Interrupts =
	__asm__ __volatile__ ("cli");
	
	// Get next PID
	pid = proc_nextPid++;
	
	proc = malloc(sizeof(t_proc));
	
	// === Clone Page Directory ===
	//#if DEBUG
	//	LogF(" Proc_Fork: Cloning Page Tables...");
	//#endif
	proc->paging_id = MM_CloneCurrent();
	//#if DEBUG
	//	LogF(" Done.\n");
	//#endif
	
	// === Fill Process Structure ===
	proc->slots = proc->timeRem = PROC_DEF_PRI;
	proc->endBss = gpCurrentProc->endBss;
	proc->pid = pid;
	proc->status = PROCSTAT_ACTIVE;
	proc->ppid = proc_pid;
	proc->esp = proc->ebp = 0;
	proc->next = NULL;
	proc->uid = gpCurrentProc->uid;
	proc->gid = gpCurrentProc->gid;
	proc->ImageName = "CHILD";
	
	// Add to task list
	tmpproc = gpProcsActive;
	while(tmpproc->next)	tmpproc = tmpproc->next;
	tmpproc->next = proc;
	
	// = Read ESP and EBP
	__asm__ __volatile__ ("mov %%esp, %0" : "=r" (proc->esp) );
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (proc->ebp) );
	eip = getEip();
	// Check if is child
	if(eip == MAGIC_SWITCH) {
		outportb(0x20, 0x20);
		return 0;
	}
	proc->eip = eip;
	//#if DEBUG
	//	LogF(" Proc_Fork: Parent\n");
	//#endif
	
	__asm__ __volatile__ ("sti");
	// = Return Child PID =
	return pid;
}

/*
int Proc_KFork()
- Creates a process that is completely empty save for
  kernel data/code
*/
int Proc_KFork()
{
	 int	pid;
	Uint	eip;
	t_proc	*proc, *tmpproc;
	
	if(gpCurrentProc == NULL) {
		warning("Proc_KFork - No Current Process (Has Multitasking Started?)\n");
		return -1;
	}

	// = Clear Interrupts =
	__asm__ __volatile__ ("cli");
	
	pid = proc_nextPid++;
	
	proc = malloc(sizeof(t_proc));
	
	//LogF("Proc_KFork: proc=*0x%x\n", proc);
	//LogF("Proc_KFork: &proc->next=0x%x\n", &proc->next);
	
	// === Clone Page Directory ===
	proc->paging_id = MM_CloneKernel();
	
	// === Fill Process Structure ===
	proc->slots = proc->timeRem = PROC_DEF_PRI;
	proc->endBss = gpCurrentProc->endBss;
	proc->pid = pid;
	proc->status = PROCSTAT_ACTIVE;
	proc->ppid = proc_pid;
	proc->esp = proc->ebp = 0;
	proc->next = NULL;
	proc->uid = gpCurrentProc->uid;
	proc->gid = gpCurrentProc->gid;
	proc->ImageName = "NULL";
	
	// Add to task list
	tmpproc = gpProcsActive;
	while(tmpproc->next)	tmpproc = tmpproc->next;
	tmpproc->next = proc;
	
	// = Read ESP and EBP
	__asm__ __volatile__ ("mov %%esp, %0" : "=r" (proc->esp) );
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (proc->ebp) );
	eip = getEip();
	// Check if is child
	if(eip == MAGIC_SWITCH) {
		outportb(0x20, 0x20);
		return 0;
	}
	proc->eip = eip;
	
	__asm__ __volatile__ ("sti");
	// = Return Child PID =
	return pid;
}

int Proc_Execve(char *path, char **argv, char **envp, int dpl)
{
	Uint	ebss;
	 int	i, j;
	 int	argc = 0;
	Uint	iEntryPoint;
	Uint	iArgument;
	Uint	*stacksetup;
	Uint	*newArgv;
	Uint16	ss, cs;
	char	*strPtr;
	
	//#if DEBUG
	//	LogF("Proc_Execve: (path='%s',argv=**0x%x,envp=**0x%x,dpl=%i)\n", path, argv, envp, dpl);
	//#endif
	
	// Set Image Name
	gpCurrentProc->ImageName = (char*) malloc( strlen(path)+1 );
	strcpy(gpCurrentProc->ImageName, path);
	// Set Image End
	gpCurrentProc->endBss = 0;
	
	// == Cache argv and envp ==
	/// \todo Fix this code to properly parse and cache data
	j = 0;	// Total Data Length
	strPtr = 0;	// Array Lengths
	if(argv) {
		for(i=0;argv[i];i++) {
			j += strlen(argv[i])+1;
		}
		j += sizeof(Uint)*(i+1);
		strPtr += sizeof(Uint)*(i+1);
		argc = i;
	}
	if(envp) {
		for(i=0;envp[i];i++)
			j += strlen(envp[i])+1;
		j += sizeof(Uint)*(i+1);
		
		strPtr += sizeof(Uint)*(i+1);
	}
	// Allocate Space
	newArgv = malloc(j);
	if(newArgv == NULL) {
		warning("Proc_Execve - Unable to allocate temporary argument/environment buffer (%i bytes)\n", j);
		return 0;
	}
	// Re-initalise variables
	strPtr += (Uint)newArgv;	// String Data Start
	j = 0;
	if(argv) {
		for(i=0;argv[i];i++,j++)
		{
			newArgv[j] = (Uint)strPtr;
			strcpy(strPtr, argv[i]);
			strPtr += strlen(argv[i]) + 1;
		}
		newArgv[j++] = 0;
	}
	// Parse Envp
	if(envp) {
		for(i=0;envp[i];i++,j++)
		{
			//LogF("envp[%i] = '%s'\n", i, envp[i]);
			newArgv[j] = (Uint)strPtr;
			strcpy(strPtr, envp[i]);
			strPtr += strlen(envp[i]) + 1;
		}
	} //*/
	//#if DEBUG
	//	LogF(" Proc_Execve: Arguments Loaded\n");
	//#endif
	
	//#if DEBUG
	//	LogF(" Proc_Execve: Clearing Memory Space\n");
	//#endif
	// == Clear Memory Space
	MM_Clear();
	//#if DEBUG
	//	LogF(" Proc_Execve: Memory Cleared, Loading Binary\n");
	//#endif
	
	// = Load Binary =
	iArgument = Binary_Load(gpCurrentProc->ImageName, &iEntryPoint);
	if(iArgument == 0) {
		warning("Unable to load image '%s'\n", gpCurrentProc->ImageName);
		Proc_Exit();
		return 0;
	}
	
	//#if DEBUG
	//	LogF(" Proc_Execve: Allocating Stack...\n");
	//#endif
	// = Allocate User Stack =
	stacksetup = (Uint *)Proc_MakeUserStack();
	// Sanity Check Return
	if(!stacksetup) {
		warning("Proc_Execve - No space for user's stack\n");
		Proc_Exit();
		return 0;
	}
	
	#if DEBUG
		LogF("Filling Stack.\n");
	#endif
	// = Create Command Line Arguments =
	for(i = 0; i < (Uint)strPtr - (Uint)newArgv + 0xFFF; i += 0x1000)
		mm_alloc(ARG_HOME+i);
	memcpy((void*)ARG_HOME, newArgv, (Uint)strPtr - (Uint)newArgv);
	free(newArgv);
	
	// == Get Descriptors ==
	switch(dpl&3)
	{
	case 0:		ss = 0x10;	cs = 0x08;	break;
	default:	ss = 0x23;	cs = 0x1B;	break;
	}

	//Command Line Arguments & Environment
	*--stacksetup = ARG_HOME+(argc+1)*sizeof(Uint);	// envp
	*--stacksetup = ARG_HOME;	// argv
	*--stacksetup = argc;		// argc
	*--stacksetup = iArgument;	// Interpreter Argument
	*--stacksetup = (Uint)User_ReturnExit;	// User Return
	
	//IRET
	ebss = (Uint)stacksetup;
	*--stacksetup = ss;		//Stack Segment
	*--stacksetup = ebss;	//Stack Pointer
	*--stacksetup = 0x0202;	//EFLAGS (Resvd (0x2) and IF (0x20))
	*--stacksetup = cs;		//Code Segment
	*--stacksetup = iEntryPoint;	//EIP
	//PUSHAD
	*--stacksetup = 0xAAAAAAAA;	// eax
	*--stacksetup = 0xCCCCCCCC;	// ecx
	*--stacksetup = 0xDDDDDDDD;	// edx
	*--stacksetup = 0xBBBBBBBB;	// ebx
	*--stacksetup = 0xD1D1D1D1;	// edi
	*--stacksetup = 0x54545454;	// esp - NOT POPED
	*--stacksetup = 0x51515151;	// esi
	*--stacksetup = 0xB4B4B4B4;	// ebp
	//Individual PUSHs
	*--stacksetup = ss;	// ds
	*--stacksetup = ss;	// es
	*--stacksetup = ss;	// fs
	*--stacksetup = ss;	// gs
	
	//#if DEBUG
	//	LogF(" Proc_Execve: Starting Process - Passing Control to 0x%x\n", iEntryPoint);
	//#endif
	// = Jump to process =
	Proc_ProcessStart(stacksetup);
	
	return 1;
}

/*
int Proc_Open(char *filename, char **argv, char **envp, int term)
- Open `filename` as a process passing `argv` as the command line arguments
  and `envp` as environment variables. Also provides file handles to
  stdout and stdin on the virtual terminal `term`.
*/
int Proc_Open(char *filename, char **argv, char **envp)
{
	int newPid;
	//#if DEBUG
	//	LogF("Proc_Open: (filename='%s')\n", filename);
	//#endif
	
	// Check if file exists
	newPid = vfs_open(filename, 0);
	if(newPid == -1)
		return -1;
	vfs_close(newPid);
	
	newPid = Proc_Fork();	//!< \todo This should be KFork, but there seems to be a bug in it.
	//newPid = Proc_KFork();
	//#if DEBUG
	//	LogF(" Proc_Open: newPid = %i\n", newPid);
	//#endif
	if(newPid == 0)
	{
		Proc_Execve(filename, argv, envp, 3);	//Start in user mode
		Proc_Exit();
		return 1;	//NEVER REACHED
	}
	if(newPid > 0)
		return newPid;
	
	return -1;
}

void Proc_Yield()
{
	#if DEBUG >= 2
	LogF("%i (%s) Yielded\n", gpCurrentProc->pid, gpCurrentProc->ImageName);
	#endif
	if(gpCurrentProc)
	{
		//#if 1
		gpCurrentProc->timeRem = 0;
		//#if 0
		//__asm__ __volatile__ ("int $0xF0");	// Call Timer
		//#elif 1
		__asm__ __volatile__ ("sti;hlt");	// Halt CPU until IRQ
		//#else
		//while(gpCurrentProc->timeRem == 0);
		//#endif
		//#else
		//__asm__ __volatile__ ("hlt");
		//#endif
	}
	else
	{
		__asm__ __volatile__ ("sti;hlt");
	}
}

/**
 \fn void Proc_Delay(int ms)
 \brief Sets a process to delay for a set number of ticks
*/
void Proc_Delay(int ms)
{
	t_procWait	*ent;
	t_procWait	*new;
	t_proc	*tmpproc;
	int		totalDelay = 0;
	
	//LogF("Proc_Delay: (ms=%i)\n", ms);
	ms *= TICKS_PER_MS;
	ms ++;

	//LogF(" Proc_Delay: ms = %i\n", ms);
	// Just use `time_wait` before multitasking is up
	if(gpCurrentProc == NULL)
	{
		warning("Please use time_wait before multitasking is up\n");
		time_wait(ms);
		return;
	}
	
	// Create Delay Entry
	new = malloc(sizeof(t_procWait));
	new->proc = gpCurrentProc;
	if(gpProcsWaiting)
	{
		ent = gpProcsWaiting;
		// Get required slot in wait list
		while(ent->next)
		{
			if(totalDelay + ent->delta > ms)
				break;
			totalDelay += ent->delta;
			ent = ent->next;
		}
		new->delta = ms-totalDelay;
		new->next = ent->next;
		ent->next = new;
		if(new->next)	new->next->delta -= new->delta;
	}
	else
	{
		new->delta = ms;
		new->next = NULL;
		gpProcsWaiting = new;
		//LogF("Proc_Delay: gpProcsWaiting->delta = %i\n", new->delta);
	}
	
	// Check if this is the top process on active stack
	if(gpProcsActive && gpProcsActive != gpCurrentProc)
	{
		// If Not, walk stack
		tmpproc = gpProcsActive;
		while(tmpproc && tmpproc->next != gpCurrentProc)
			tmpproc = tmpproc->next;
		if(!tmpproc)
			panic("Proc_Delay: Unable to find process on active queue\n");
		tmpproc->next = gpCurrentProc->next;
	}
	else
	{	// Remove from stack
		gpProcsActive = gpCurrentProc->next;
	}
	
	// Yeild Timeslice
	Proc_Yield();
}

/**
 \fn void Proc_Sleep()
 \brief Sets a process to sleep until it receives a signal.
*/
void Proc_Sleep()
{
	t_proc	*tmpproc, *thisproc;
	thisproc = gpCurrentProc;
	
	#if DEBUG
	//LogF("Proc_Sleep: PID=%i\n", proc_pid);
	#endif
	
	if(thisproc->status == PROCSTAT_SLEEP)	// This shouldn't happen
		return;
	
	thisproc->status = PROCSTAT_SLEEP;
	
	if(!gpProcsActive)
		panic("Proc_Sleep - Active Stack Empty. ==BUG CATCHER==");
	
	// Check if this is the top process on active stack
	if(gpProcsActive != thisproc)
	{
		// If Not, walk stack
		tmpproc = gpProcsActive;
		while(tmpproc && tmpproc->next != thisproc)
			tmpproc = tmpproc->next;
		if(!tmpproc)
			panic("Proc_Sleep - Unable to find process on active queue\n");
		tmpproc->next = thisproc->next;
	}
	else
	{
		// Remove from stack
		gpProcsActive = thisproc->next;
	}
	
	ll_append((void**)&gpProcsAsleep, T_PROC_NEXT_OFS, thisproc);
	//Add to Sleeping queue
	//tmpproc = gpProcsAsleep;
	//while(tmpproc && tmpproc->next)	tmpproc = tmpproc->next;
	//if(tmpproc)	tmpproc->next = thisproc;
	//else	tmpproc = thisproc;

	#if DEBUG
	LogF("Proc_Sleep: %i (%s) is napping\n", proc_pid, thisproc->ImageName);
	#endif
	Proc_Yield();
	//__asm__ __volatile__ ("hlt");
}

/**
 \fn int Proc_Kill(int pid)
 \brief Kills the selected process
*/
int Proc_Kill(int pid)
{
	t_proc	*proc, *tmpproc;
	proc = gpProcsActive;
	while(proc && proc->pid != pid)	proc = proc->next;
	if(!proc)
	{
		proc = gpProcsAsleep;
		while(proc && proc->next && proc->next->pid != pid)	proc = proc->next;
		
		tmpproc = proc->next;
		proc->next = tmpproc->next;
		proc = tmpproc;
	}
	if(!proc)
	{
		t_procWait	*pw;
		pw = gpProcsWaiting;
		while(pw && pw->proc->pid != pid)	pw = pw->next;
		if(pw)
			proc = pw->proc;
	}
	if(!proc)
		return -1;
		
	// Mark for death
	proc->status = PROCSTAT_NULL;
	free(gpCurrentProc->ImageName);
	// Remove Memory Space
	mm_remove(proc->paging_id);
	return 1;
}

/**
 \fn void Proc_Exit()
 \brief Kills the current process
*/
void Proc_Exit()
{
	t_proc	*tmpproc, *thisproc;
	thisproc = gpCurrentProc;
	
	// We're messing with the process queue here, so don't allow interupts
	__asm__ __volatile__ ("cli");
	
	// Check if this is the top process on active stack
	if(gpProcsActive && gpProcsActive != thisproc)
	{
		// If Not, walk stackTian van Heerden wrote, "this message came up on the automated parking
		tmpproc = gpProcsActive;
		while(tmpproc && tmpproc->next != thisproc)
			tmpproc = tmpproc->next;
		if(tmpproc)
			tmpproc->next = thisproc->next;
	}
	else
	{
		// Remove from stack
		gpProcsActive = thisproc->next;
	}
	
	// Check if this is the top process on sleeping stack
	if(gpProcsAsleep && gpProcsAsleep != thisproc)
	{
		// If Not, walk stack
		tmpproc = gpProcsAsleep;
		while(tmpproc && tmpproc->next != thisproc)
			tmpproc = tmpproc->next;
		if(tmpproc)
			tmpproc->next = thisproc->next;
	}
	else
	{
		// Remove from stack
		gpProcsAsleep = thisproc->next;
	}
	
	mm_remove(thisproc->paging_id);
	
	// Mark for removal
	thisproc->status = PROCSTAT_NULL;
	// Renable Interrupts (If we get stopped here it doesn't matter
	__asm__ __volatile__ ("sti;\n\thlt");
}

void Proc_WakeProc(int pid)
{
	t_proc	*proc, *tmpproc;
	
	proc = gpProcsAsleep;
	if(!proc)	return;	//SegFault Protection
	if(proc->pid == pid)
	{
		proc->status = PROCSTAT_ACTIVE;
		gpProcsAsleep = proc->next;
		ll_append((void**)&gpProcsActive, T_PROC_NEXT_OFS, proc);
		return;
	}
	
	while(proc->next && proc->next->pid != pid)
		proc = proc->next;
	
	if(!proc->next)	return;
	
	proc->next->status = PROCSTAT_ACTIVE;
	
	tmpproc = gpProcsActive;
	if(!tmpproc) {
		gpProcsActive = proc->next;
		proc->next = proc->next->next;
		return;
	}
	
	while(tmpproc->next)
		tmpproc = tmpproc->next;
	tmpproc->next = proc->next;
	proc->next = proc->next->next;
}

int Proc_GetPid()
{
	return gpCurrentProc->pid;
}
int Proc_GetUid()
{
	return gpCurrentProc->uid;
}
int Proc_GetGid()
{
	return gpCurrentProc->gid;
}
int	Proc_WaitPid(int pid, int action)
{
	t_proc	*proc;
	
	proc = gpProcsActive;
	while(proc && proc->pid != pid)
		proc = proc->next;
	if(!proc)	return -1;
	
	switch(action)
	{
	case 0:	// Die
		while(proc->pid == pid && proc->status != PROCSTAT_NULL)
			Proc_Yield();
		return 0;
		break;
	}
	return 0;
}

/* Extend process memory space
 */
Uint32 proc_brk(Uint32 newEnd)
{
	int increase;
	int	pageDelta;
	Uint32 newAddress;
	
	if(newEnd == 0)
		return gpCurrentProc->endBss;
	
	// Calculate Increase in size
	increase = newEnd - gpCurrentProc->endBss;
	
	if(increase < 0)
	{
		increase -= 0x1000 - (gpCurrentProc->endBss&0xFFF);
		increase = -increase;
		pageDelta = (increase+0xFFF)>>12;
		newAddress = (gpCurrentProc->endBss&0xFFFFF000);
		while(pageDelta--)
		{
			if(!MM_IsValid(newAddress))
				return gpCurrentProc->endBss;
			mm_dealloc(newAddress);
			newAddress -= 0x1000;
		}
		return (gpCurrentProc->endBss = newAddress);
	}
	
	//Within a page
	if(increase+(gpCurrentProc->endBss&0xFFF) <= 0x1000) {
		gpCurrentProc->endBss = newEnd;
		return newEnd;
	}
	
	//Crossing Page Boundaries
	increase -= 0x1000 - (gpCurrentProc->endBss&0xFFF);
	pageDelta = (increase+0xFFF)>>12;
	newAddress = (gpCurrentProc->endBss&~0xFFF);
	
	while(pageDelta-->=0)
	{
		if(MM_IsValid(newAddress))
			continue;
		//	return gpCurrentProc->endBss;
		mm_alloc(newAddress);
		newAddress += 0x1000;
	}
	
	gpCurrentProc->endBss = newEnd;
	return newEnd;
}

/**
 \fn Uint Proc_MakeUserStack()
*/
Uint Proc_MakeUserStack()
{
	 int	j,i;
	Uint	base = STACK_TOP;
	for(j=STACK_COUNT;j--;)
	{
		base -= STACK_SIZE<<12;
		// Check Prospective Space
		for(i=STACK_SIZE;i--;)	if( MM_IsValid(base+(i<<12)) )	break;
		if(i!=-1)	continue;	// If a page was taken go to next space
		// Allocate Stack
		for(i=0;i<STACK_SIZE;i++)	mm_alloc(base+(i<<12));	// Incrementally Allocate to clean up dump
		break;
	}
	if(j==-1)	return 0;
	return base + (STACK_SIZE << 12);
}

/**
 \fn void Proc_DumpRunning()
*/
void Proc_DumpRunning()
{
	t_proc	*proc;
	t_procWait	*ent;
	LogF("Active Processes:\n");
	proc = gpProcsActive;
	while(proc)
	{
		if(proc == gpCurrentProc)
			LogF("#");
		else
			LogF(" ");
		LogF(" PID %i: '%s', Parent=%i, UID:%i, GID:%i, Next=0x%x\n",
			proc->pid, proc->ImageName, proc->ppid, proc->uid, proc->gid, proc->next);
		if(proc == proc->next)
			break;
		proc = proc->next;
	}
	
	LogF("Sleeping Processes:\n");
	proc = gpProcsAsleep;
	while(proc)
	{
		LogF("  PID %i: '%s', Parent=%i, UID:%i, GID:%i, Next=0x%x\n",
			proc->pid, proc->ImageName, proc->ppid, proc->uid, proc->gid, proc->next);
		if(proc == proc->next)
			break;
		proc = proc->next;
	}
	
	LogF("Waiting Processes:\n");
	ent = gpProcsWaiting;
	while(ent)
	{
		proc = ent->proc;
		LogF("  PID %i: '%s', Parent=%i, UID:%i, GID:%i, Wait=%i\n",
			proc->pid, proc->ImageName, proc->ppid, proc->uid, proc->gid, ent->delta);
		if(ent == ent->next)
			break;
		ent = ent->next;
	}
	
}

/**
 * \fn void proc_scheduler(Uint stack)
 * \brief Called on timer and swaps processes
 * \param stack	ESP on calling (Just passed to time_handler)
*/
void proc_scheduler(Uint stack)
{
	Uint32	eip, esp, ebp;
	
	// NOTE: This should always be first
	// = Read Esp, Ebp, Eip =
	__asm__ __volatile__ ("mov %%esp, %0" : "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0" : "=r"(ebp));
	eip = getEip();
	
	// = Check if returning =
	if(eip == MAGIC_SWITCH)	return;
	
	// Call Timer Handler
	time_handler( (struct regs *)&stack );
	
	#if DEBUG >= 2	// Debug Text
		puts("Scheduler.\n");
	#endif
	
	// === Wake Delayed Processes ===
	// While wueue is not empty and delta on first entry is zero
	while(gpProcsWaiting && gpProcsWaiting->delta == 0)
	{
		void	*next;
		#if DEBUG
		LogF("Restore #%i\n", gpProcsWaiting->proc->pid);
		#endif
		// Append to active queue
		gpProcsWaiting->proc->next = gpProcsActive;	// Set Next to current top
		gpProcsActive = gpProcsWaiting->proc;	// Set active to proc
		// Remove from wait queue
		next = gpProcsWaiting->next;	// Save next in list
		free(gpProcsWaiting);	// Free old entry
		gpProcsWaiting = next;	// Set new entry
	}
	if(gpProcsWaiting) {
		#if DEBUG
		LogF("gpProcsWaiting->delta = %i\n", gpProcsWaiting->delta);
		#endif
		gpProcsWaiting->delta --;
	}
	
	// == Check for Idling (No Current Process) ==
	if(!gpCurrentProc)
	{
		// Check if the active queue is empty
		if(!gpProcsActive) {
			// If so, ACK IRQ and halt CPU
			#if DEBUG
				puts("proc_scheduler: Idling\n");
			#endif
			outportb(0x20, 0x20);
			__asm__ __volatile__ ("sti;hlt" : : );	//Halt the system until next interupt
		}
	}
	else	// Process currently running
	{
		// -- Check if the current process has time remaining --
		if(gpCurrentProc->timeRem > 0) {
			// Decrement time and return
			gpCurrentProc->timeRem --;
			return;
		}
	
		#if DEBUG >= 2
			puts("Swapping Processes\n");
		#endif
	
		// = Update EIP, ESP, EBP =
		gpCurrentProc->eip = eip;
		gpCurrentProc->esp = esp;
		gpCurrentProc->ebp = ebp;

		// = Check if the current process is about to be killed
		if(gpCurrentProc->status == PROCSTAT_NULL)
		{
			t_proc	*tmpp;
			
			#if DEBUG
			LogF("proc_scheduler: Destroy %i\n", gpCurrentProc->pid);
			#endif

			// Remove From Active Queue
			tmpp = gpProcsActive;
			while(tmpp && tmpp->next != gpCurrentProc)	tmpp = tmpp->next;
			if(tmpp)
			{
				tmpp->next = gpCurrentProc->next;
				// Wake all processes that are waiting on us
				// --- TODO ---
				// Free
				tmpp = gpCurrentProc->next;
				free(gpCurrentProc);
				gpCurrentProc = tmpp;
			}
			gpCurrentProc = NULL;
		} else {
			// Go to next process
			gpCurrentProc = gpCurrentProc->next;
		}
	}
	
	// = Check for the end of the active queue =
	if(!gpCurrentProc)	gpCurrentProc = gpProcsActive;
	
	// = Check for an Active Process = 
	if(!gpCurrentProc || gpCurrentProc->status != PROCSTAT_ACTIVE)
	{
		#if DEBUG >= 2
		puts("No active processes, Idling\n");
		#endif
		// = Send EOI to Master Controller =
		outportb(0x20, 0x20);
		__asm__ __volatile__ ("sti;hlt" : : );	//Halt the system until next interupt
		return;
	}
	
	// = Update Process Time =
	gpCurrentProc->timeRem = gpCurrentProc->slots;
	#if DEBUG >= 2
	LogF("Starting gpCurrentProc->timeRem = %i\n", gpCurrentProc->timeRem);
	#endif
	
	// = Read Registers =
	eip = gpCurrentProc->eip;
	esp = gpCurrentProc->esp;
	ebp = gpCurrentProc->ebp;
	#if SCHED_SANITY_CHECK
	if(eip < 0x1000) {
		warning("PID%i was trashed (EIP = 0x%x)", gpCurrentProc->pid, eip);
		outportb(0x20, 0x20);
		__asm__ __volatile__ ("hlt" : : );
		return;
	}
	#endif
	
	#if DEBUG >= 2
		LogF(" Proc_Sceduler: eip=0x%x,esp=0x%x,ebp=0x%x\n", eip, esp, ebp);
	#endif
	
	// Change Memory Space
	MM_SetPid(gpCurrentProc->paging_id);
	// Set new Process PID
	proc_pid = gpCurrentProc->pid;
	//if(proc_pid == 1)	__asm__ __volatile__ ("xchg %bx, %bx");

	//MM_Dump();	// DEBUG

	//Perform Task Switch
	__asm__ __volatile__ (
	"cli;\n\t"		//Clear Interrupts
	"mov %2, %%esp;\n\t"	//Set ESP
	"mov %3, %%ebp;\n\t"	//Set EBP
	"sti;\n\t"		//Start Interrupts
	"jmp *%%ecx" : : "a" (MAGIC_SWITCH), "c" (eip), "r" (esp), "r" (ebp) );
	
	return;
}

/**
 * \fn void tss_setup()
 * \brief Sets the TSS up for multitasking
 */
void tss_setup()
{
	int i = 0;
	Uint32	base;
	
	memset(&proc_tss, 0, sizeof(t_proc_tss)*MAX_CPUS);
	
	#if MAX_CPUS > 1
	for(i=MAX_CPUS;i--;)
	#endif
	{
		base = ((Uint32)&proc_tss[i]);
		proc_tss[i].ss0 = 0x10;
		proc_tss[i].esp0 = 0xF0000000 - 4;
		//proc_tss[i].esp0 = (Uint)&Proc_Stack_Top - 4;
		//proc_tss[i].ss = 0x10|3;
		//proc_tss[i].cs = 0x08|3;
		//proc_tss[i].ds = proc_tss[i].es = proc_tss[i].gs = proc_tss[i].fs = 0x10|3;
		proc_tss[i].iobitmap = sizeof(t_proc_tss);
		//gdt_set_gate(5+i, base, sizeof(t_proc_tss)-1, 0xE9, 0x00);
		gdt_set_gate(5+i, base, sizeof(t_proc_tss)-1, 0x89, 0x00);
	}
}
