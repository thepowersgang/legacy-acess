# Acess Window Manager version 1
#  AxWin
#  Makefile

CC = i686-pc-elf-gcc.exe
AS = nasm_32
RM = @rm -f
LD = i686-pc-elf-ld.exe
OBJDUMP = i686-pc-elf-objdump.exe

AOBJ = start.ao
COBJ = main.o wm.o graphics.o desktop.o
BIN = ../axwin.elf

CFLAGS = -Wall -nostdinc -fno-builtin -fleading-underscore -I../libc/include
ASFLAGS = -felf
LDFLAGS = --oformat elf32-i386 -L.. -lc -Map map.txt -I /Mount/hda1/Acess/Libs/ld-acess.so


.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(AOBJ) $(COBJ)

$(BIN): $(AOBJ) $(COBJ) ../libc.so
	$(LD) $(LDFLAGS) -o $(BIN) $(AOBJ) $(COBJ) > link.txt
	$(OBJDUMP) -d $(BIN) > axwin.dsm
	..\cpy2dsk.bat ..\axwin.elf axwin.elf

$(AOBJ): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(COBJ): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	