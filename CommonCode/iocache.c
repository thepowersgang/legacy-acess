/*
 AcessOS Version 1
 By thePowersGang

 I/O Cache
*/
#include <acess.h>

// === STRUCTURES ===
typedef struct sCacheEntry {
	Uint	DevID;
	Uint	TTL;	// Indicates the latency of the device
	Uint64	SubID;
	Uint64	LastRead;
	Uint64	LastWrite;	// 0 for Clean
	Uint8	Data[512];
} tCacheEntry;	// Sizeof = 544 bytes
typedef struct sCacheDevice {
	Uint	DevID;
	 int	(*write)(Uint dev, Uint subId);
	struct sCacheDevice	*Next;
} tCacheDevice;

// === GLOBALS ===
Uint	gIOCache_NextId = 1;
 int	gIOCache_MaxSize = 32;	// Max 32 Blocks

// === CODE ===
/**
 \fn Uint IOCache_GetID()
*/
Uint IOCache_GetID(int(*writeFcn)(Uint, Uint))
{
	return gIOCache_NextId++;
}

