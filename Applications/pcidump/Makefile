# Project: Acess Shell

CC = gccX
AS = nasm_32
LD = ldX
RM = @rm -f
OBJDUMP = /mingw/bin/objdump.exe

AOBJ = start.ao ../syscalls.ao
COBJ = main.o lib.o
BIN = ../pcidump.axe

CFLAGS = -Wall -O -fstrength-reduce -nostdinc -fno-builtin -I./include
ASFLAGS = -felf
LDFLAGS = -T link.ld

.PHONY : all clean

all: $(BIN)

$(BIN): $(AOBJ) $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $(AOBJ) $(COBJ) -M > Map.txt
	$(OBJDUMP) -d $@ > $@.dsm

clean :
	$(RM) $(AOBJ) $(COBJ) $(BIN)

$(COBJ): %.o: %.c
	@echo === GCC -c $? -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

$(AOBJ): %.ao: %.asm
	@echo === NASM $? -o $@
	$(AS) $(ASFLAGS) -o $@ $<
