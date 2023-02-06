#Project: Acess OS FDD Driver
#CC = gccX
CC = \mingw\bin\gcc
#LD = ldX.exe
LD = \mingw\bin\ld
OBJDUMP = \mingw\bin\objdump
AS = nasm

#CFLAGS = -Wall -fPIC -O -fstrength-reduce -nostdinc -fno-builtin -I.. -g
CFLAGS = -Wall -O -fstrength-reduce -nostdinc -fno-builtin -I.. -g
ASFLAGS = -f elf
#LDFLAGS = -T ../link.ld -shared
#LDFLAGS = -shared --error-unresolved-symbols
#LDFLAGS = -shared -pie -Map map.txt
LDFLAGS = -T ../pe.ld -Map map.txt --dll --image-base 0xD0000000

OBJ = ../start_pe.ao fdd.o
BIN = kmod_fdd.akm

.PHONY: all clean

all: $(OBJ) $(BIN)

$(filter %.ao,$(OBJ)): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(filter %.o,$(OBJ)): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ) 
	$(LD) $(LDFLAGS) $(OBJ) -o $(BIN)
	$(OBJDUMP) -S -d $(BIN) > $(BIN).dsm
	
