/*
AcessOS Basic LibC
Malloc.c - Heap Manager
*/
#include <syscalls.h>
#include <stdlib.h>
#include "lib.h"

// === Constants ===
#define NULL	((void*)(0))
#define MAGIC	0xACE55051	//AcessOS1
#define MAGIC_FREE	(~MAGIC)	//AcessOS1
#define BLOCK_SIZE	16	//Minimum
#define HEAP_INIT_SIZE	0x10000

typedef unsigned int Uint;

//Typedefs
typedef struct {
	Uint	magic;
	Uint	size;
}	heap_head;
typedef struct {
	heap_head	*header;
	Uint	magic;
}	heap_foot;

//Globals
void	*heap_start = NULL;
void	*heap_end = NULL;

//Prototypes
EXPORT void	*malloc(Uint bytes);
EXPORT void	free(void *mem);
EXPORT void	*realloc(Uint bytes, void *mem);
EXPORT void	*sbrk(int increment);
LOCAL void	*extendHeap(int bytes);

//Code

/**
 \fn EXPORT void *malloc(size_t bytes)
 \brief Allocates memory from the heap space
 \param bytes	Integer - Size of buffer to return
 \return Pointer to buffer
*/
EXPORT void *malloc(size_t bytes)
{
	Uint	bestSize;
	Uint	closestMatch = 0/*, foundFree = 0*/;
	Uint	bestMatchAddr = 0;
	heap_head	*curBlock = heap_start;
	
	if(heap_start == NULL) {
		heap_start = sbrk(0);
		heap_end = heap_start;
		extendHeap(HEAP_INIT_SIZE);
		curBlock = heap_start;
	}
	
	bestSize = bytes + sizeof(heap_head) + sizeof(heap_foot) + BLOCK_SIZE - 1;
	bestSize = (bestSize/BLOCK_SIZE)*BLOCK_SIZE;	//Round up to block size
	
	while((Uint)curBlock < (Uint)heap_end) {
		if(curBlock->magic == MAGIC_FREE) {
			//foundFree = 1;
			if(curBlock->size == bestSize)
				break;
			if(bestSize < curBlock->size && (curBlock->size < closestMatch || closestMatch == 0)) {
				closestMatch = curBlock->size;
				bestMatchAddr = (Uint)curBlock;
			}
		} else if(curBlock->magic != MAGIC) {
			//Corrupt Heap
			return NULL;
		}
		curBlock = (heap_head*)((Uint)curBlock + curBlock->size);
	}
	
	if((Uint)curBlock < (Uint)heap_start) {
		//panic("malloc: Heap underrun for some reason\n");
		return NULL;
	}
	
	//Found a perfect match
	if((Uint)curBlock < (Uint)heap_end) {
		curBlock->magic = MAGIC;
		return (void*)((Uint)curBlock + sizeof(heap_head));
	}
	
	//Out of Heap Space
	if(!closestMatch) {
		curBlock = extendHeap(bestSize);	//Allocate more
		if(curBlock == NULL) {
			//panic("malloc: Out of kernel heap memory\n");
			return NULL;
		}
		curBlock->magic = MAGIC;
		return (void*)((Uint)curBlock + sizeof(heap_head));
	}
	
	//Split Block?
	if(closestMatch - bestSize > BLOCK_SIZE) {
		heap_foot	*foot;
		curBlock = (heap_head*)bestMatchAddr;
		curBlock->magic = MAGIC;
		curBlock->size = bestSize;
		foot = (heap_foot*)(bestMatchAddr + bestSize - sizeof(heap_foot));
		foot->header = curBlock;
		foot->magic = MAGIC;

		curBlock = (heap_head*)(bestMatchAddr + bestSize);
		curBlock->magic = MAGIC_FREE;
		curBlock->size = closestMatch - bestSize;
		
		foot = (heap_foot*)(bestMatchAddr + closestMatch - sizeof(heap_foot));
		foot->header = curBlock;
		
		((heap_head*)bestMatchAddr)->magic = MAGIC;	//mark as used
		return (void*)(bestMatchAddr + sizeof(heap_head));
	}
	
	//Don't Split the block
	((heap_head*)bestMatchAddr)->magic = MAGIC;
	return (void*)(bestMatchAddr+sizeof(heap_head));
}

/**
 \fn EXPORT void free(void *mem)
 \brief Free previously allocated memory
 \param mem	Pointer - Memory to free
*/
EXPORT void free(void *mem)
{
	heap_head	*head = mem;
	
	if(head->magic != MAGIC)	//Valid Heap Address
		return;
	
	head->magic = MAGIC_FREE;
	
	//Unify Right
	if((Uint)head + head->size < (Uint)heap_end)
	{
		heap_head	*nextHead = (heap_head*)((Uint)head + head->size);
		if(nextHead->magic == MAGIC_FREE) {	//Is the next block free
			head->size += nextHead->size;	//Amalgamate
			nextHead->magic = 0;	//For Security
		}
	}
	//Unify Left
	if((Uint)head - sizeof(heap_foot) > (Uint)heap_start)
	{
		heap_head	*prevHead;
		heap_foot	*prevFoot = (heap_foot *)((Uint)head - sizeof(heap_foot));
		if(prevFoot->magic == MAGIC) {
			prevHead = prevFoot->header;
			if(prevHead->magic == MAGIC_FREE) {
				prevHead->size += head->size;	//Amalgamate
				head->magic = 0;	//For Security
			}
		}
	}
}

/**
 \fn EXPORT void *realloc(size_t bytes, void *oldPos)
 \brief Reallocate a block of memory
 \param bytes	Integer - Size of new buffer
 \param oldPos	Pointer - Old Buffer
 \return Pointer to new buffer
*/
EXPORT void *realloc(size_t bytes, void *oldPos)
{
	void *ret;
	heap_head	*head;
	
	if(oldPos == NULL) {
		return malloc(bytes);
	}
	
	//Check for free space after block
	head = (heap_head*)((Uint)oldPos-sizeof(heap_head));
	
	//Hack to used free's amagamating algorithym and malloc's splitting
	free(oldPos);
	
	//Allocate new memory
	ret = malloc(bytes);
	if(ret == NULL)
		return NULL;
	
	//Copy Old Data
	if((Uint)ret != (Uint)oldPos) {
		memcpy(ret, oldPos, head->size-sizeof(heap_head)-sizeof(heap_foot));
	}
	
	//Return
	return ret;
}

/**
 \fn LOCAL void *extendHeap(int bytes)
 \brief Create a new block at the end of the heap area
 \param bytes	Integer - Size reqired
 \return Pointer to last free block
 */

LOCAL void *extendHeap(int bytes)
{
	heap_head	*head = heap_end;
	heap_foot	*foot;

	//Expand Memory Space
	foot = sbrk(bytes);
	
	//Create New Block
	head->magic = MAGIC_FREE;	//Unallocated
	head->size = bytes;
	
	foot->header = head;
	foot->magic = MAGIC;
	
	//Combine with previous block if nessasary
	if(heap_end != heap_start && ((heap_foot*)((Uint)heap_end-sizeof(heap_foot)))->magic == MAGIC) {
		heap_head	*tmpHead = ((heap_foot*)((Uint)heap_end-sizeof(heap_foot)))->header;
		if(tmpHead->magic == MAGIC_FREE) {
			tmpHead->size += bytes;
			foot->header = tmpHead;
			head = tmpHead;
		}
	}
	
	heap_end = (void*) ((Uint)foot+sizeof(heap_foot));
	return head;
}

/**
 \fn EXPORT void *sbrk(int increment)
 \brief Increases the program's memory space
 \param count	Integer - Size of heap increase
 \return Pointer to start of new region
*/
EXPORT void *sbrk(int increment)
{
	size_t newEnd;
	static size_t oldEnd = 0;
	static size_t curEnd = 0;

	if (oldEnd == 0)	curEnd = oldEnd = brk(0);

	if (increment == 0)	return (void *) curEnd;

	newEnd = curEnd + increment;

	if (brk(newEnd) == curEnd)	return (void *) -1;
	oldEnd = curEnd;
	curEnd = newEnd;

	return (void *) oldEnd;
}
