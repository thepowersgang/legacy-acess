# Acess Window Manager version 1
#  AxWin
#  Makefile

CC = gccX
AS = nasm_32
RM = @rm -f
LD = ldX
OBJDUMP = objdump

AOBJ = start.ao syscalls.ao
COBJ = main.o wm.o graphics.o desktop.o heap.o
# stdlib.o
BIN = ..\axwin.axe

CFLAGS = -Wall -nostdinc -fno-builtin -I. -I..
ASFLAGS = -felf
LDFLAGS = -T ../axe.ld


.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(AOBJ) $(COBJ)

$(BIN): $(AOBJ) $(COBJ) ../libc.so
	$(LD) $(LDFLAGS) -o $(BIN) $(AOBJ) $(COBJ) -Map map.txt > link.txt
	..\cpy2dsk.bat $(BIN) axwin.axe
	$(OBJDUMP) -D --start-address=0x1C -bbinary -mi386 ..\axwin.axe > axwin.dsm

$(AOBJ): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(COBJ): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	