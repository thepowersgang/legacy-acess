/*
AcessOS Version 1
Windows Interface Layer
KERNEL32.DLL
*/
#include <syscalls.h>
#include <dll.h>
#include <winapi.h>

// === PROTOTYPES ===
DLL_EXPORT w32ATOM	AddAtomA(LPCSTR str);

// === TYPES ===
/*typedef struct s_lATOM_TABLE {
	struct s_lATOM_TABLE	*Next;
	CWCHAR	Str[];
} t_lATOM_TABLE;*/

// === GLOBAL VARIABLES ===
//t_lATOM_TABLE	*guAtomTable = NULL;

// === CODE ===
/**
 \fn DLL_EXPORT w32ATOM AddAtomA(LPCSTR str)
 \brief Add an item to global atom table
 \return 0 for failure, id elsewise
*/
w32ATOM	AddAtomA(LPCSTR str)
{
	_write(0, 10, "AddAtomA\n");
	return 0;
}
