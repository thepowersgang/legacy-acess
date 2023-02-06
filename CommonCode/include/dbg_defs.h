/*
Acess v1
Debugging Definitions
*/
#ifndef _DBG_DEF_H
#define _DBG_DEF_H

#if DEBUG
# define	DEBUGS(v...)	LogF(v)
#else
# define	DEBUGS(v...)
#endif

#endif
