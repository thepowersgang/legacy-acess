/*
 AcessOS PCI Dump
/// LIB.C \\\
*/
#include "header.h"

// STRINGS
int strcmp(char *s1, char *s2)
{
	while(*s1 == *s2 && *s1 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

void strcpy(char *dst, char *src)
{
	while(*src) {
		*dst = *src;
		src++; dst++;
	}
	*dst = '\0';
}

int strlen(char *str)
{
	int retval;
	for(retval = 0; *str != '\0'; str++)
		retval++;
	return retval;
}


const char cUCDIGITS[] = "0123456789ABCDEF";
//itoa
void itoa(char *buf, unsigned long num, int base)
{
	char tmpBuf[32];
	int pos=0;
	int	i = 0;
	//Loop Digits
	while(num > base-1) {
		tmpBuf[pos] = cUCDIGITS[ num % base ];
		pos++;
		num = (long) num / base;		//Shift {number} right 1 digit
	}
	
	tmpBuf[pos++] = cUCDIGITS[ num % base ];		//Last digit of {number}
	for(;pos--;) buf[i++] = tmpBuf[pos];	//Reverse the order of characters
	buf[i] = '\0';
}
void itoas(char *buf, signed long num, int base)
{
	char tmpBuf[32];
	int pos=0;
	int	i = 0;
	int	sign = 0;
	
	if(num < 0) {
		sign = 1;
		num = -num;
	}
	
	//Loop Digits
	while(num > base-1) {
		tmpBuf[pos++] = cUCDIGITS[ num % base ];
		num = (long) num / base;		//Shift {number} right 1 digit
	}
	tmpBuf[pos++] = cUCDIGITS[ num % base ];		//Last digit of {number}
	
	if(sign)	buf[i++] = '-';
	
	for(;pos--;) buf[i++] = tmpBuf[pos];	//Reverse the order of characters
	buf[i] = '\0';
}


static char printfBuf[128];
void printf(char *fmt, ...)
{
	char	tmp[33];
	 int	c, arg;
	 int	pos = 0;
	char	*p;
	 int	*args = (int*)((unsigned int)&fmt+4);

	tmp[32] = '\0';
	
	while((c = *fmt++) != 0)
	{
		if (c != '%')
			printfBuf[pos++] = c;
		else {
        
			c = *fmt++;
			if(c == '%') {
				printfBuf[pos++] = '%';
				continue;
			}
		
			p = tmp;
		
			arg = *args++;
			switch (c) {
            case 'd':
            case 'i':
				itoas(p, arg, 10);
				goto sprintf_puts;
			//	break;
            case 'u':
				itoa(p, arg, 10);
				goto sprintf_puts;
			//	break;
            case 'x':
				itoa(p, arg, 16);
				goto sprintf_puts;
			//	break;
            case 'o':
				itoa(p, arg, 8);
				goto sprintf_puts;
			//	break;
            case 'b':
				itoa(p, arg, 2);
				goto sprintf_puts;
			//	break;

            case 's':
				p = (void*)arg;
			sprintf_puts:
				if(!p)	p = "(null)";
				while(*p)	printfBuf[pos++] = *p++;
				break;

			default:
				printfBuf[pos++] = arg;
				break;
            }
        }
    }
	
	printfBuf[pos++] = '\0';
	write(_stdout, pos, printfBuf);
}

// FILE PATHS
int GeneratePath(char *file, char *base, char *tmpPath)
{
	char	*pathComps[64];
	char	*tmpStr;
	int		iPos = 0;
	int		iPos2 = 0;
	
	//Print("GeneratePath: (file=\"");Print(file);Print("\", base=\"");Print(base);Print("\")\n");
	
	// Parse Base Path
	if(file[0] != '/')
	{
		pathComps[iPos++] = tmpStr = base+1;
		while(*tmpStr)
		{
			if(*tmpStr++ == '/')
			{
				pathComps[iPos] = tmpStr;
				iPos ++;
			}
		}
	}
	
	//Print(" GeneratePath: Base Done\n");
	
	// Parse Argument Path
	pathComps[iPos++] = tmpStr = file;
	while(*tmpStr)
	{
		if(*tmpStr++ == '/')
		{
			pathComps[iPos] = tmpStr;
			iPos ++;
		}
	}
	pathComps[iPos] = NULL;
	
	//Print(" GeneratePath: Path Done\n");
	
	// Cleanup
	iPos2 = iPos = 0;
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		// Always Increment iPos
		iPos++;
		// ..
		if(tmpStr[0] == '.' && tmpStr[1] == '.'	&& (tmpStr[2] == '/' || tmpStr[2] == '\0') )
		{
			if(iPos2 != 0)
				iPos2 --;
			continue;
		}
		// .
		if(tmpStr[0] == '.' && (tmpStr[1] == '/' || tmpStr[1] == '\0') )
		{
			continue;
		}
		// Empty
		if(tmpStr[0] == '/' || tmpStr[0] == '\0')
		{
			continue;
		}
		
		// Set New Position
		pathComps[iPos2] = tmpStr;
		iPos2++;
	}
	pathComps[iPos2] = NULL;
	
	//Print(" GeneratePath: Cleaned\n");
	//Print(" GeneratePath: New\n");
	
	// Build New Path
	iPos2 = 1;	iPos = 0;
	tmpPath[0] = '/';
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		while(*tmpStr && *tmpStr != '/')
		{
			tmpPath[iPos2++] = *tmpStr;
			tmpStr++;
		}
		tmpPath[iPos2++] = '/';
		iPos++;
	}
	if(iPos2 > 1)
		tmpPath[iPos2-1] = 0;
	else
		tmpPath[iPos2] = 0;
	
	//Print(" GeneratePath: Complete\n");
	
	return iPos2;	//Length
}
