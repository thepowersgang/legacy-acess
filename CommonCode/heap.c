/*
Acess OS
Heap Manager v1
*/
#include <acess.h>

#define HEAP_DEBUG	0
#define DEBUG (HEAP_DEBUG | ACESS_DEBUG)

#define MAGIC	0xACE55051	//AcessOS1
#define MAGIC_FREE	(~0xACE55051)	//531AAFAE
#define BLOCK_SIZE	32	//Minimum (16 Data, 16 Header)
#define INIT_HEAP_SIZE	4096	//1 Page Initial Heap

//Typedefs
typedef struct {
	Uint32	magic;
	Uint32	size;
}	heap_head;
typedef struct {
	heap_head	*header;
	Uint32	magic;
}	heap_foot;

//Globals
void	*gpGlobalHeapStart;
void	*gpGlobalHeapEnd;
void	*gpLocalHeapStart;
void	*gpLocalHeapEnd;

//Prototypes
void Heap_Init();
void *malloc(Uint32 bytes);
void free(void *mem);
void *realloc(Uint32 bytes, void *mem);
void *ExtendHeap(int bytes, void *heapStart, void **heapEnd);

//Code
/* Initialise Heap
 */
void Heap_Init()
{
	gpGlobalHeapStart = (void*)0xC0400000;
	gpGlobalHeapEnd = (void*)0xC0400000;

	gpLocalHeapStart = (void*)0xE0000000;
	gpLocalHeapEnd = (void*)0xE0000000;

	//mm_alloc((Uint)gpLocalHeapStart);
	ExtendHeap(INIT_HEAP_SIZE, gpLocalHeapStart, &gpLocalHeapEnd);

	//mm_alloc((Uint)gpGlobalHeapStart);
	ExtendHeap(INIT_HEAP_SIZE, gpGlobalHeapStart, &gpGlobalHeapEnd);
	printf("Heap Initialised\n");
}

/*
 void *AllocAt(Uint32 bytes, void *heapStart, void *heapEnd)
 - Allocates Memory Block from a specific heap
*/
void *AllocAt(Uint32 bytes, void *heapStart, void **heapEnd)
{
	Uint	bestSize;
	Uint	closestMatch = -1;
	Uint	bestMatchAddr = 0;
	heap_head	*curBlock = heapStart;
	heap_foot	*foot;
	
	#if DEBUG
		LogF("AllocAt: (bytes=%u, heapStart=*0x%x, heapEnd=*0x%x)\n", bytes, heapStart, *heapEnd);
	#endif
	
	// Calculate Minimum Block size
	bestSize = ( bytes + sizeof(heap_head) + sizeof(heap_foot) + BLOCK_SIZE - 1) / BLOCK_SIZE;
	bestSize *= BLOCK_SIZE;
	
	// Walk the heap space
	while((Uint)curBlock < (Uint)*heapEnd)
	{
		#if DEBUG & 2
			LogF(" AllocAt: curBlock=*0x%x\n", curBlock);
			LogF(" AllocAt: curBlock={magic=0x%x,size=0x%x}\n", curBlock->magic, curBlock->size);
		#endif
		// Is the Block Free?
		if(curBlock->magic == MAGIC_FREE)
		{
			//foundFree = 1;
			if(curBlock->size == bestSize)
				break;
			if(bestSize < curBlock->size && curBlock->size < closestMatch)
			{
				closestMatch = curBlock->size;
				bestMatchAddr = (Uint)curBlock;
			}
		}
		// Is the Magic Valid
		else if(curBlock->magic != MAGIC)
			panic("AllocAt - Heap has been trashed, magic value was overwritten.\n Address 0x%x (Phys: 0x%x)", curBlock, MM_GetPhysAddr((Uint)curBlock));
		// Sanity Check the size
		if(curBlock->size & 3)
			panic("AllocAt - Heap has been trashed, size is not dword divisible.\n Address 0x%x (Phys: 0x%x)", &curBlock->size, MM_GetPhysAddr((Uint)&curBlock->size) );
		// Check footer
		foot = (heap_foot*)((Uint)curBlock + curBlock->size - sizeof(heap_foot));
		if( foot->header != curBlock )
			panic("AllocAt - Heap has been trashed, footer backlink is not valid.\n Address 0x%x\n"
				" Is: 0x%x, Needed: 0x%x",
				&foot->header, foot->header, curBlock);
		if( foot->magic != MAGIC )
			panic("AllocAt - Heap has been trashed, footer magic value is invalid.\n Address 0x%x (Phys: 0x%x)",
				&foot->magic, MM_GetPhysAddr((Uint)&foot->magic) );
		curBlock = (heap_head*)((Uint)curBlock + curBlock->size);
	}
	
	// Check for heap underrun
	if((Uint)curBlock < (Uint)heapStart)
	{
		panic("AllocAt - Heap underrun for some reason\n");
	}
	
	//Found a perfect match
	if((Uint32)curBlock < (Uint32)*heapEnd)
	{
		#if DEBUG
			LogF(" AllocAt: Found a perfect match - Return: %x\n", (curBlock + sizeof(heap_head)));
		#endif
		if(curBlock->magic != MAGIC_FREE)
			panic("AllocAt - Heap traversal seemed to have failed, suspect trashing.\n Address: 0x%x (Phys: 0x%x)\n", &curBlock->magic, MM_GetPhysAddr((Uint)&curBlock->magic) );
		
		curBlock->magic = MAGIC;
		return (void*)((Uint32)curBlock + sizeof(heap_head));
	}
	
	//Out of Heap Space
	if(closestMatch == -1)
	{
		curBlock = ExtendHeap(bestSize, heapStart, heapEnd);	//Allocate more
		if(curBlock == NULL) {
			panic("AllocAt - Out of kernel heap memory\n");
		}
		curBlock->magic = MAGIC;
		#if DEBUG
			LogF(" AllocAt: Extended Heap - Return: %x\n", (Uint32)curBlock + sizeof(heap_head) );
		#endif
		return (void*)((Uint32)curBlock + sizeof(heap_head));
	}
	
	//Split Block?
	if(closestMatch - bestSize > BLOCK_SIZE)
	{
		#if DEBUG
			LogF(" AllocAt: Resizing Block at %x, size %i\n", bestMatchAddr, closestMatch);
			LogF("  => New Size %i\n", bestSize);
		#endif
		curBlock = (heap_head*)bestMatchAddr;
		if(curBlock->magic != MAGIC_FREE)
			panic("AllocAt - Heap traversal seemed to have failed, suspect trashing.\n Address: 0x%x (Phys: 0x%x)\n", &curBlock->magic, MM_GetPhysAddr((Uint)&curBlock->magic) );
		curBlock->magic = MAGIC;
		curBlock->size = bestSize;
		foot = (heap_foot*)(bestMatchAddr + bestSize - sizeof(heap_foot));
		foot->header = curBlock;
		foot->magic = MAGIC;
		
		curBlock = (heap_head*)(bestMatchAddr + bestSize);
		#if DEBUG
			LogF(" AllocAt: curBlock = *0x%x\n", (Uint)curBlock);
		#endif
		curBlock->magic = MAGIC_FREE;
		curBlock->size = closestMatch - bestSize;
		#if DEBUG
			LogF(" AllocAt: curBlock->size = 0x%x\n", (Uint)curBlock->size);
		#endif
		
		foot = (heap_foot*)(bestMatchAddr + closestMatch - sizeof(heap_foot));
		foot->header = curBlock;
		if( foot->magic != MAGIC )
			panic("AllocAt - Heap trashed (foot).\n Address: 0x%x (Phys: 0x%x)\n", &foot->magic, MM_GetPhysAddr((Uint)&foot->magic) );
		foot->magic = MAGIC;
		
		#if DEBUG
			LogF(" AllocAt: Resized Block - Return: %x\n", bestMatchAddr + sizeof(heap_head));
		#endif
		return (void*)(bestMatchAddr + sizeof(heap_head));
	}
	
	if(((heap_head*)bestMatchAddr)->magic != MAGIC_FREE)
		panic("AllocAt - Heap traversal seemed to have failed, suspect trashing.\n Address: 0x%x (Phys: 0x%x)\n", &curBlock->magic, MM_GetPhysAddr((Uint)&curBlock->magic) );
	//Don't Split the block
	((heap_head*)bestMatchAddr)->magic = MAGIC;
	#if DEBUG
		LogF(" AllocAt: Did not resize block - Return: %x\n", bestMatchAddr+sizeof(heap_head));
	#endif
	return (void*)(bestMatchAddr+sizeof(heap_head));
}

/*
Free Memory in specified Heap
*/
void DeallocAt(void *mem, void *heapStart, void **heapEnd)
{
	heap_head	*head;
	
	#if DEBUG
		LogF("DeallocAt: (mem=*0x%x,heapStart=0x%x,heapEnd=*0x%x)\n", mem, heapStart, *heapEnd);
	#endif
	
	if(mem == NULL)	return;
	
	head = mem - sizeof(heap_head);
	
	if(head->magic != MAGIC)	//Valid Heap Address
	{
		#if DEBUG
			LogF(" DeallocAt: Invalid (Already Freed?)\n");
		#endif
		return;
	}
	
	#if DEBUG
		LogF(" DeallocAt: head->size=0x%x, head->magic=0x%x\n", head->size, head->magic);
	#endif
		
	head->magic = MAGIC_FREE;
	
	//Unify Right
	if((Uint)head + head->size < (Uint)*heapEnd)
	{
		heap_head	*nextHead = (heap_head*)((Uint)head + head->size);
		heap_foot	*nextFoot;
		if(nextHead->magic == MAGIC_FREE)	//Is the next block free
		{
			nextFoot = (heap_foot*)((Uint)nextHead + nextHead->size - sizeof(heap_foot));
			nextFoot->header = head;
			#if DEBUG
				LogF(" DeallocAt: head->size (%x) += nextHead->size (%x) : %x\n",
					head->size, nextHead->size, head->size + nextHead->size);
			#endif
			head->size += nextHead->size;	//Amalgamate
			nextHead->magic = 0;	//For Security
		}
	}
	//Unify Left
	if((Uint)head > (Uint)heapStart)
	{
		heap_head	*prevHead;
		heap_foot	*prevFoot = (heap_foot *)((Uint)head - sizeof(heap_foot));
		if(prevFoot->magic == MAGIC)
		{
			prevHead = prevFoot->header;
			if(prevHead->magic == MAGIC_FREE)
			{
				prevFoot = (heap_foot *)((Uint)head + head->size - sizeof(heap_foot));
				prevFoot->header = prevHead;
				prevHead->size += head->size;	//Amalgamate
				head->magic = 0;	//For Security
			}
		} else
			panic("DeallocAt - Heap trashed, Footer magic is invalid (0x%x)\n Address: 0x%x\n",
				prevFoot->magic, &prevFoot->magic);
	}
}

/*
 Create a new block at the end of the heap area
*/
void *ExtendHeap(int bytes, void *heapStart, void **heapEnd)
{
	int	pageDelta;
	Uint	newAddress;
	Uint	bssEnd = (Uint) *heapEnd;
	heap_head	*head = *heapEnd;
	heap_foot	*foot = (heap_foot*)( (Uint32)*heapEnd + bytes - sizeof(heap_foot));

	#if DEBUG
		LogF("ExtendHeap: (bytes=%i,heapStart=0x%x,heapEnd=0x%x)\n", bytes, heapStart, *heapEnd);
	#endif
	
	// Make sure byte count is multiple of block size
	if(bytes % BLOCK_SIZE != 0)
		bytes += BLOCK_SIZE - bytes % BLOCK_SIZE;
	
	//Expand Kernel Memory Space
	if((bssEnd & 0xFFF) == 0)
	{
		pageDelta = (bytes + 0xFFF) >> 12;
		newAddress = ( bssEnd + 0xFFF ) & 0xFFFFF000;
		while(pageDelta--)
		{
			mm_alloc(newAddress);
			newAddress += 0x1000;
		}
	}
	else if(bytes + (0x1000-(bssEnd & 0xFFF)) > 0x1000)
	{
		//Crossing Page Boundaries
		pageDelta = ( (bytes - (0x1000 - (bssEnd & 0xFFF) ) + 0xFFF) >> 12);
		newAddress = ( bssEnd + 0xFFF ) & 0xFFFFF000;
		while(pageDelta--)
		{
			mm_alloc(newAddress);
			newAddress += 0x1000;
		}
	}
	
	//Create New Block
	head->magic = MAGIC_FREE;	//Unallocated
	head->size = bytes;
	
	foot->header = head;
	foot->magic = MAGIC;
	
	//Combine with previous block if nessasary
	if(*heapEnd != heapStart && ((heap_foot*)(bssEnd - sizeof(heap_foot)))->magic == MAGIC)
	{
		heap_head	*tmpHead = ((heap_foot*)(bssEnd - sizeof(heap_foot)))->header;
		if(tmpHead->magic == MAGIC_FREE)
		{
			tmpHead->size += bytes;
			foot->header = tmpHead;
			head = tmpHead;
		}
	}
	
	#if DEBUG
		LogF("ExtendHeap: RETURN 0x%x\n", head);
	#endif
	
	*heapEnd = (void*) ((Uint)foot + sizeof(heap_foot));
	return head;
}

/*
Allocate Memory from Global Heap
*/
void *malloc(Uint32 bytes)
{
	return AllocAt(bytes, gpGlobalHeapStart, &gpGlobalHeapEnd);
}
/*
Free Memory from Global Heap
*/
void free(void *mem)
{
	DeallocAt(mem, gpGlobalHeapStart, &gpGlobalHeapEnd);
}
/*
Allocate Memory from the Local Heap
*/
void *AllocLocal(Uint32 bytes)
{
	return AllocAt(bytes, gpLocalHeapStart, &gpLocalHeapEnd);
}
/*
Free Memory from the Local Heap
*/
void DeallocLocal(void *mem)
{
	DeallocAt(mem, gpLocalHeapStart, &gpLocalHeapEnd);
}

/* Reallocate a block of memory
 */
void *realloc(Uint32 bytes, void *oldPos)
{
	void *ret;
	heap_head	*head, *nextHead;
	Uint	reqdSize;
	Uint	oldSize;
	
	#if DEBUG
	printf("realloc: (bytes=%i, oldPos=*0x%x)\n", (Uint)bytes, (Uint)oldPos);
	#endif	
	
	if(oldPos == NULL)
	{
		return malloc(bytes);
	}
	
	head = (heap_head*)((Uint)oldPos - sizeof(heap_head));
	
	oldSize = head->size - sizeof(heap_head) - sizeof(heap_foot);
	
	reqdSize = (bytes + (BLOCK_SIZE-1) + sizeof(heap_head) + sizeof(heap_foot)) / BLOCK_SIZE;
	reqdSize *= BLOCK_SIZE;
	
	// Fast Return - Buffer has not changed in size (substantially)
	if(reqdSize == head->size)
		return oldPos;
	
	// Shrink buffer if needed (only do if saving half buffer space)
	// TODO: Optomize this to find happy medium
	if(reqdSize < head->size)
	{
		// Size change is not large enough
		if(reqdSize > head->size / 2)	return oldPos;
		heap_foot	*foot;
		nextHead = (heap_head *)( (Uint)head+reqdSize );
		nextHead->size = head->size - reqdSize;
		
		foot = (heap_foot*)( (Uint)nextHead - sizeof(heap_foot) );
		foot->magic = MAGIC;
		foot->header = head;
		
		foot = (heap_foot*)( (Uint)head + head->size - sizeof(heap_foot) );
		foot->magic = MAGIC;
		foot->header = nextHead;
		
		nextHead->magic = MAGIC_FREE;
		head->size = reqdSize;
	}
	
	#if 0
	// Expand Buffer and avoid malloc and free etc.
	nextHead = (heap_head*)((Uint)head+head->size);
	if( nextHead->magic == MAGIC_FREE && head->size+nextHead->size >= reqdSize )
	{
		heap_foot	*foot;
		int	delta = head->size+nextHead->size - reqdSize;
		LogF("realloc: delta = 0x%x\n", delta);
		if(delta > 0)
		{
			nextHead = (heap_head *)( (Uint)head + reqdSize );
			nextHead->magic = MAGIC_FREE;
			nextHead->size = delta;
			foot = (heap_foot *)( (Uint)nextHead + delta - sizeof(heap_foot) );
			foot->magic = MAGIC;
			foot->header = nextHead;
			nextHead = (heap_head*)((Uint)head + head->size);
		}
		head->size = reqdSize;
		foot = (heap_foot *)( (Uint)head + reqdSize - sizeof(heap_foot) );
		foot->header = head;
		foot->magic = MAGIC;
		nextHead->magic = 0;
		return oldPos;
	}
	#endif
	
	//Hack to used free's amagamating algorythm and malloc's splitting
	free(oldPos);
	
	//Allocate new memory
	ret = malloc(bytes);
	if(ret == NULL)
		return NULL;
	
	// Same Value Return
	if(ret == oldPos)
		return ret;
	
	//LogF(" realloc: ret=0x%x,oldPos=0x%x\n", ret, oldPos);
	//LogF(" realloc: bytes=0x%x,oldSize=0x%x\n", bytes, oldSize);
	
	//Copy Old Data
	if((Uint)ret > (Uint)oldPos)
	{
		if(oldSize < bytes)
			memcpyda(ret, oldPos, oldSize/4);
		else
			memcpyda(ret, oldPos, bytes/4);
	} else {
		Uint	*d, *s;
		if(oldSize < bytes) {
			d = (Uint*)((Uint)ret + oldSize - 4);
			s = (Uint*)((Uint)oldPos + oldSize - 4);
			for(;oldSize;oldSize-=4)	*--d = *--s;
		} else {
			d = (Uint*)((Uint)ret + bytes);
			s = (Uint*)((Uint)oldPos + bytes);
			for(;bytes;bytes-=4)	*--d = *--s;
		}
	}
	
	//Return
	return ret;
}
