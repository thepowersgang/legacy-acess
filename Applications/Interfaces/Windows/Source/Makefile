# AcessOS Version 1
# Windows Interface DLLs
# Makefile

CC = gcc.exe
LD = ld.exe
RM = @rm -f
OBJDUMP = objdump.exe
DLLTOOL = dlltool.exe

CFLAGS = -Wall -Werror -fstrength-reduce -nostdinc -fno-builtin -I./include -I..
LDFLAGS = -shared -nostdlib

OBJS = kernel32.dll

.PHONY:	all

all: $(OBJS)

$(OBJS:.dll=.o): %.o: %.c include/dll.h
	$(CC) -c $< -o $@ $(CFLAGS)
	
$(OBJS): %.dll: %.o %.def
	$(CC) $(LDFLAGS) -o $@ $?
	$(OBJDUMP) -x $@ > $@.dmp

# Dependencies
kernel32.c: ../syscalls.h
