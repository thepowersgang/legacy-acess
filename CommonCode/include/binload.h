/*
Acess v0.1
Binary Loader Code
HEADER
*/
#ifndef _BINLOAD_H
#define _BINLOAD_H

struct sLoadedBin {
	char	*TruePath;
	char	*Interpreter;
	Uint	Base;
	Uint	EntryPoint;
	 int	ReferenceCount;
	// int	PageSpace;
	 int	PageCount;
	struct sLoadedBin	*Next;
	struct {
		Uint	Virtual;
		Uint	Physical;
	}	Pages[];
};
// --- Flags (Encoded in lowest bits of Pages->Virtual ---
#define	LB_PAGE_READONLY	0x1
#define	LB_PAGE_EXECUTE		0x2

struct sProcessBin {
	struct sLoadedBin	*Info;
};


typedef struct sLoadedBin	tLoadedBin;

// === EXPORTS/IMPOTRS ===
extern char	*Binary_RegInterp(char *path);

#endif
