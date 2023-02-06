# Project: AcessOS
# Unique Code

CC = gcc
AS = nasm
RM = @rm -f
LD = ld
OBJDUMP = objdump
AOBJ = start.ao assembler.ao user.ao
COBJ = main.o desctab.o proc.o mm2.o vm8086.o
CFLAGS = -Wall -O -fstrength-reduce -fleading-underscore -nostdinc -fno-builtin -I./include -I../CommonCode/include
ASFLAGS = -f elf32
LDFLAGS = -T link.ld
BIN = "../AcessOS.bin"

.PHONY : all clean

all: $(BIN)

clean :
	$(RM) $(AOBJ) $(COBJ) $(BIN)

$(BIN): $(AOBJ) $(COBJ)
#	$(LD) $(LDFLAGS) -o $(BIN) start.ao ../CommonCode/export.ao assembler.ao user.ao $(COBJ) $(CCOBJ) -M > ../OSMap.txt
	@echo --- Linking ---
	@$(LD) $(LDFLAGS) -o $(BIN) start.ao ../CommonCode/export.ao assembler.ao user.ao *.o ../CommonCode/*.o -M >../OSMap.txt
	@$(OBJDUMP) -d -S $(BIN) > ../AcessOS.dsm
	@$(OBJDUMP) -x $(BIN) > ../AcessOS.dmp

$(COBJ): %.o: %.c
	@echo === GCC -c $? -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

$(AOBJ): %.ao: %.asm
	@echo === NASM $? -o $@
	@$(AS) $< -o $@ $(ASFLAGS)
