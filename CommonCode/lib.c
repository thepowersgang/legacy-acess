/*
AcessOS v0.1
Basic Library Functions
*/
#include <acess.h>

#define USE_REP	1

// ==== GLOBALS ====
const char cHEX_DIGITS[] = "0123456789ABCDEF";

// === CODE ===
/**
 \fn void *memcpy(void *dest, const void *src, Uint count)
 \brief Copys data from one memory location to another
 \param dest	pointer - destination
 \param src	pointer - source
 \param count	integer - Number of bytes to copy
*/
void *memcpy(void *dest, const void *src, Uint count)
{
	#if USE_REP
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"movsb;\n\t"
		"popf;\n\t"
		: : "c" (count), "S" (src), "D" (dest) );
	#else
    const char *sp = (const char *)src;
    char *dp = (char *)dest;
    for(;count--;) *dp++ = *sp++;
	#endif
    return dest;
}

/**
 \fn void memcpyd(void *dst, void *src, Uint dwCount)
 \brief Copys data from one memory location to another (32bit version)
 \param dst	pointer - Destination
 \param src	pointer - Source
 \param dwCount	integer - Number of DWORDS to copy
*/
void memcpyd(void *dst, void *src, Uint dwCount)
{
	for(;dwCount--;) {
		*((Uint32*)dst) = *((Uint32*)src);
		dst += 4;	src += 4;
	}
}

/**
 \fn void memcpyda(void *dst, void *src, Uint count)
 \brief Copy data in 32 bit chunks when data is aligned
*/
void memcpyda(void *dst, void *src, Uint count)
{
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"movsl;\n\t"
		"popf;\n\t"
		: : "c" (count), "S" (src), "D" (dst) );
}

/**
 \fn void *memset(void *dest, char val, Uint count)
 \brief Fills a memory location with the stated value
 \param dest	pointer - destination
 \param val	byte - Fill Value
 \param count	integer - Number of bytes to set
*/
void *memset(void *dest, char val, Uint count)
{
    char *temp = (char *)dest;
    for(;count--;) *temp++ = val;
    return dest;
}
void *memsetw(void *dest, Uint16 val, Uint count)
{
    Uint16 *temp = (Uint16 *)dest;
    for(;count--;) *temp++ = val;
    return dest;
}
void *memsetd(void *dest, Uint32 val, Uint count)
{
	Uint32	*temp = (Uint32*)dest;
	while(count--) *temp++ = val;
	return dest;
}

/**
 \fn void memsetda(void *dst, Uint32 val, Uint count)
 \brief Set data in 32 bit chunks when data is aligned
*/
void memsetda(void *dst, Uint32 val, Uint count)
{
	__asm__ __volatile__ (
		"pushf;\n\t"
		"cld;\n\t"
		"rep;\n\t"
		"stosl;\n\t"
		"popf;\n\t"
		: : "c" (count), "a" (val), "D" (dst) );
}

int strlen(const char *str)
{
	int retval;
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

//Case insensitive String Compare
int strncmp(const char *str1, const char *str2)
{
	int cmp1, cmp2;
	cmp1 = *str1;	cmp2 = *str2;
	if('a' <= cmp1 && cmp1 <= 'z')	cmp1 -= 0x20;
	if('a' <= cmp2 && cmp2 <= 'z')	cmp2 -= 0x20;
	while(cmp1 == cmp2 && *str1 != '\0' && *str2 != '\0') {
		str1++; str2++;
		cmp1 = *str1;	cmp2 = *str2;
		if('a' <= cmp1 && cmp1 <= 'z')	cmp1 -= 0x20;
		if('a' <= cmp2 && cmp2 <= 'z')	cmp2 -= 0x20;
	}
	return cmp1 - cmp2;
}


void strcpy(char *dest, const char *src)
{
	while(*src) {
		*dest = *src;
		src++; dest++;
	}
	*dest = '\0';	// End Destination String
}

//FROM io.c
/* Read 1 Byte from an IO port
 */
Uint8 inportb (Uint16 _port)
{
	Uint8 rv;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}

/* Read 1 word from an IO port
 */
Uint16 inportw (Uint16 _port)
{
	Uint16 rv;
	__asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}

/* Read 1 dword from an IO port
 */
Uint32 inportd (Uint16 _port)
{
	Uint32 rv;
	__asm__ __volatile__ ("inl %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}

/* Write 1 Byte to an IO port
 */
void outportb (Uint16 _port, Uint8 _data)
{
	__asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

/* Write 1 word to an IO port
 */
void outportw (Uint16 _port, Uint16 _data)
{
	__asm__ __volatile__ ("outw %1, %0" : : "dN" (_port), "a" (_data));
}

/* Write 1 dword to an IO port
 */
void outportd (Uint16 _port, Uint32 _data)
{
	__asm__ __volatile__ ("outl %1, %0" : : "dN" (_port), "a" (_data));
}

/*
Uint HexToInt(char *str)
- Convert a Hex string to an integer
- Stops at first invalid character
*/
Uint HexToInt(char *str)
{
	Uint	ret = 0;
	int		tmp;
	for(;;str++)
	{
		if(*str < '0')	break;
		tmp = *str - '0';
		if(tmp > 9) {	//Alpha Portion
			tmp = tmp + '0' - 'A' + 10;	// Adjust so that 'A' is 10
			if(tmp > 15)	tmp -= 0x20;	// Account for capital
			if(tmp < 0)		break;	// Invalid Character
			if(tmp > 15)	break;	//     "
		}
		ret <<= 4;
		ret |= tmp;
	}
	return ret;
}
/*
Uint DecToInt(char *str)
- Convert a Decimal string to an integer
- Stops at first invalid character
*/
Uint DecToInt(char *str)
{
	Uint	ret = 0;
	int		tmp;
	for(;;str++)
	{
		if(*str < '0')	break;
		tmp = *str - '0';
		if(tmp > 9)	break;
		ret *= 10;
		ret += tmp;
	}
	return ret;
}
/**
 \fn Uint OctToInt(char *str)
 \brief Convert a Octal string to an integer
 \param str	String to parse
 \return Integer parsed
 \note Stops at first invalid character
*/
Uint OctToInt(char *str)
{
	Uint	ret = 0;
	int		tmp;
	for(;;str++)
	{
		if(*str < '0')	break;
		tmp = *str - '0';
		if(tmp > 7)	break;
		ret <<= 3;
		ret |= tmp;
	}
	return ret;
}
/**
 \fn Uint BinToInt(char *str)
 \brief Convert a Binary string to an integer
 \param str	String to parse
 \return Integer parsed
 \note Stops at first invalid character
*/
Uint BinToInt(char *str)
{
	Uint	ret = 0;
	int		tmp;
	for(;;str++)
	{
		if(*str == '0')	tmp = 0;
		else
			if(*str == '1')	tmp = 1;
			else	break;
		ret <<= 1;
		ret |= tmp;
	}
	return ret;
}

/**
 \fn void ll_append(void **start, int nextOfs, void *item)
 \brief Append an item to a linked list
 \param start	Pointer to a pointer denoting the start of the list
 \param nextOfs	Offset of pointer to next item in list
 \param item	Pointer to item to add
*/
void ll_append(void **start, int nextOfs, void *item)
{
	char	*ptr = *start;
	// Clear Next for item
	*(void**)((char*)item+nextOfs) = NULL;
	// NULL Check
	if(!ptr) {/*LogF("ll_append: Starting list (start=0x%x) with item\n", start);*/	*start = item;	return;}
	// Loop until end
	while(*(void**)(ptr+nextOfs))	ptr = *(void**)(ptr+nextOfs);
	// Append
	*(void**)(ptr+nextOfs) = item;
	*(void**)((char*)item+nextOfs) = NULL;
}

/**
 \fn void ll_delete(void **start, int nextOfs, void *item)
 \brief Delete an item from a linked list
 \param start	Pointer to a pointer denoting the start of the list
 \param nextOfs	Offset of pointer to next item in list
 \param item	Pointer to the item to delete from list
*/
void ll_delete(void **start, int nextOfs, void *item)
{
	char	*ptr = *start;
	// NULL Check
	if(!ptr)	return;
	// Check if first item
	if(ptr == item) {
		*start = *(void**)(((char*)item)+nextOfs);
		return;
	}
	// Loop until found
	while(ptr && *(void**)(ptr+nextOfs) != item)
		ptr = *(void**)(ptr+nextOfs);
	// Check if found
	if( *(void**)(ptr+nextOfs) != item )	return;
	// Delete
	*(void**)(ptr+nextOfs) = *(void**)(*(Uint*)(ptr+nextOfs)+nextOfs);
}

/**
 \fn void *ll_findId(void **start, int nextOfs, int idOfs, int id)
 \brief Find an item given an id
 \param start	Pointer to a pointer denoting the start of the list
 \param nextOfs	Offset of pointer to next item in list
 \param idOfs	Offset of item ID in list item
 \param id	ID to look for
*/
void *ll_findId(void **start, int nextOfs, int idOfs, int id)
{
	char	*ptr = *start;
	// Search
	while(ptr && *(int*)(ptr+idOfs) != id)
		ptr = *(void**)(ptr+nextOfs);
	// Return
	return ptr;
}

//==============================
//= Exported Function Pointers
//==============================
Uint32	lib_functions[] = {
	(Uint32)memcpy,	//0
	(Uint32)memcpyd,	//1
	(Uint32)memset,	//2
	(Uint32)memsetw,	//3
	
	(Uint32)strlen,	//4
	(Uint32)strcmp,	//5
	(Uint32)strncmp,	//6
	(Uint32)strcpy,	//7
	
	(Uint32)inportb,	//8
	(Uint32)inportw,	//9
	(Uint32)inportd,	//10
	(Uint32)outportb,		//11
	(Uint32)outportw,	//12
	(Uint32)outportd	//13
};
