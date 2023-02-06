#Project: Acess OS Bochs Graphics Driver
CC = /mingw/bin/gcc.exe
LD = /mingw/bin/ld.exe
OBJDUMP = /mingw/bin/objdump.exe
AS = nasm_32.exe

CFLAGS = -Wall -O -fstrength-reduce -nostdinc -fno-builtin -I..
ASFLAGS = -f elf
#LDFLAGS = -T ../elf.ld -shared -Map map.txt
LDFLAGS = -T ../pe.ld --dll --image-base 0xD0000000

OBJ = ../start_pe.ao drv_bochsvbe.o
BIN = kmod_bochsvbe.akm

.PHONY: all clean

all: $(OBJ) $(BIN)

$(filter %.ao,$(OBJ)): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(filter %.o,$(OBJ)): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ) 
	$(LD) $(LDFLAGS) $(OBJ) -o $(BIN) -M > map.txt
	$(OBJDUMP) -S -d $(BIN) > $(BIN).dsm
	
