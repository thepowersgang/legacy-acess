/*
Acess64
 Variable Arguments
 VA_ARGS.H
*/
#ifndef _VA_ARGS_H
#define _VA_ARGS_H

typedef __builtin_va_list	va_list;

#define va_start(v, l)	__builtin_va_start(v,l)
#define va_end(v)		__builtin_va_end(v)
#define va_arg(v, l)	__builtin_va_arg(v,l)

#endif
