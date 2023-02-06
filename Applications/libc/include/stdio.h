/*
AcessOS LibC

stdio.h
*/

typedef struct {
	unsigned int	KernelFP;
	unsigned int	Flags;
	unsigned int	Length;
	unsigned int	ReadPos;
	unsigned int	WritePos;
} FILE;
