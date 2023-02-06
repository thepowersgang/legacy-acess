# Project: Acess Shell

CC = gccX
AS = nasm_32
LD = ldX
RM = @rm -f

COMMON = start.ao ../syscalls.ao
BINS = ForkBomb

CFLAGS = -Wall -O -fstrength-reduce -nostdinc -fno-builtin -I./include
ASFLAGS = -felf
LDFLAGS = -T link.ld

.PHONY : all clean

all: $(BINS)

ForkBomb: ForkBomb.o $(COMMON)
	$(LD) $(LDFLAGS) -o $@ $? -M > Map.txt

clean :
	$(RM) *.o *.ao $(BINS)

ForkBomb.o: %.o: %.c
	@echo === GCC -c $? -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

$(COMMON): %.ao: %.asm
	@echo === NASM $? -o $@
	$(AS) $(ASFLAGS) -o $@ $<
