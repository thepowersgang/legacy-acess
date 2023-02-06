# Project: Acess Shell

CC = gccX
AS = nasm_32
LD = ldX
RM = @rm -f

AOBJ = start.ao ../syscalls.ao
COBJ = main.o lib.o
BIN = ../sh2.axe

CFLAGS = -Wall -O -fstrength-reduce -nostdinc -fno-builtin -I./include -I../CommonCode/include
ASFLAGS = -felf
LDFLAGS = -T link.ld

.PHONY : all clean

all: $(BIN)

$(BIN): $(AOBJ) $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $(AOBJ) $(COBJ) -M > Map.txt

clean :
	$(RM) $(AOBJ) $(COBJ) $(BIN)

$(COBJ): %.o: %.c
	@echo === GCC -c $? -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

$(AOBJ): %.ao: %.asm
	@echo === NASM $? -o $@
	$(AS) $(ASFLAGS) -o $@ $<
