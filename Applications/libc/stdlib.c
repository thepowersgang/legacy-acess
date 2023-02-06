/*
AcessOS Basic C Library

stdlib.c
*/
#include <syscalls.h>
#include <stdlib.h>
#include "lib.h"

int _stdout = 1;
int _stdin = 2;

EXPORT void	itoa(char *buf, unsigned long num, int base);
EXPORT void	itoas(char *buf, signed long num, int base);
EXPORT int	ssprintfv(char *format, va_list args);
EXPORT void	sprintfv(char *buf, char *format, va_list args);
EXPORT void	printf(char *format, ...);
EXPORT int	strlen(char *str);
EXPORT int	strcmp(char *str1, char *str2);
EXPORT void	strcpy(char *dst, char *src);

//sprintfv
/**
 \fn EXPORT void sprintfv(char *buf, char *format, va_list args)
 \brief Prints a formatted string to a buffer
 \param buf	Pointer - Destination Buffer
 \param format	String - Format String
 \param args	VarArgs List - Arguments
*/
EXPORT void sprintfv(char *buf, char *format, va_list args)
{
	char	tmp[33];
	 int	c, arg;
	 int	pos = 0;
	char *p;

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		if (c != '%')
			buf[pos++] = c;
		else {
        
			c = *format++;
			if(c == '%') {
				buf[pos++] = '%';
				continue;
			}
		
			p = tmp;
		
			arg = va_arg(args, int);
			switch (c) {
            case 'd':
            case 'i':
				itoas(tmp, arg, 10);
				goto sprintf_puts;
			//	break;
            case 'u':
				itoa(tmp, arg, 10);
				goto sprintf_puts;
			//	break;
            case 'x':
				itoa(tmp, arg, 16);
				goto sprintf_puts;
			//	break;
            case 'o':
				itoa(tmp, arg, 8);
				goto sprintf_puts;
			//	break;
            case 'b':
				itoa(tmp, arg, 2);
				goto sprintf_puts;
			//	break;

            case 's':
				p = (void*)arg;
			sprintf_puts:
				if(!p)	p = "(null)";
				while(*p)	buf[pos++] = *p++;
				break;

			default:
				buf[pos++] = arg;
				break;
            }
        }
    }
}
/*
ssprintfv
- Size, Stream, Print Formated, Variable Argument List
*/
/**
 \fn EXPORT int ssprintfv(char *format, va_list args)
 \brief Gets the total character count from a formatted string
 \param format	String - Format String
 \param args	VarArgs - Argument List
*/
EXPORT int ssprintfv(char *format, va_list args)
{
	char	tmp[33];
	 int	c, arg;
	 int	len = 0;
	char	*p;

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		if (c != '%')
			len++;
		else {
			c = *format++;
			
			if(c == '%') {
				len++;
				continue;
			}
			p = tmp;
			
			arg = va_arg(args, int);
			switch (c) {			
            case 'd':
            case 'i':
				itoas(tmp, arg, 10);
				goto sprintf_puts;
            case 'u':
				itoa(tmp, arg, 10);
				goto sprintf_puts;
            case 'x':
				itoa(tmp, arg, 16);
				goto sprintf_puts;
            case 'o':
				itoa(tmp, arg, 8);
				p = tmp;
				goto sprintf_puts;
            case 'b':
				itoa(tmp, arg, 2);
				goto sprintf_puts;

            case 's':
				p = (void*)arg;
			sprintf_puts:
				if(!p)	p = "(null)";
				while(*p)	len++, p++;
				break;

			default:
				len ++;
				break;
            }
        }
    }
	return len;
}

const char cUCDIGITS[] = "0123456789ABCDEF";
//itoa
EXPORT void itoa(char *buf, unsigned long num, int base)
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
	
	tmpBuf[pos] = cUCDIGITS[ num % base ];		//Last digit of {number}
	for(;pos--;) buf[i++] = tmpBuf[pos];	//Reverse the order of characters
}
EXPORT void itoas(char *buf, signed long num, int base)
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
}

//printf
EXPORT void printf(char *format, ...)
{
	int	size;
	char	*buf;
	va_list	args;
	
	va_start(args, format);
	size = ssprintfv(format, args);
	va_end(args);
	
	buf = (char*)malloc(size+1);
	buf[size] = '\0';
	
	va_start(args, format);
	sprintfv(buf, format, args);
	va_end(args);
	
	
	write(_stdout, size+1, buf);
	
	free(buf);
}

EXPORT void sprintf(char *buf, char *format, ...)
{
	va_list	args;
	va_start(args, format);
	return sprintfv(buf, format, args);
	va_end(args);
}


//MEMORY
EXPORT int strcmp(char *s1, char *s2)
{
	while(*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}
EXPORT void strcpy(char *dst, char *src)
{
	while(*src) {
		*dst = *src;
		src++; dst++;
	}
	*dst = '\0';
}
EXPORT int strlen(char *str)
{
	int retval;
	for(retval = 0; *str != '\0'; str++)
		retval++;
	return retval;
}
EXPORT void *memcpy(void *dest, void *src, unsigned int count)
{
    char *sp = (char *)src;
    char *dp = (char *)dest;
    for(;count--;) *dp++ = *sp++;
    return dest;
}
