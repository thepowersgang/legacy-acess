# Project: Acess
# Common Code

CC = gcc
AS = nasm
RM = @rm -f
AOBJ = export.ao

COBJ = system.o heap.o lib.o irq.o time.o syscalls.o scrn.o debug.o config.o \
		vfs.o fs_rom.o fs_fat.o fs_devfs2.o fs_internal.o \
		binload.o bin_elf.o \
		dma.o drv_pci.o drv_ps2mouse.o drv_vbe.o drv_ne2k.o \
		drv_ide.o drv_fdd.o keyboard_v2.o drv_tty.o
CFLAGS = -Wall -O -fstrength-reduce -fleading-underscore -fno-stack-protector -nostdinc -fno-builtin -I./include -I../AcessOS/include
ASFLAGS = -felf

.PHONY : all clean

all: $(AOBJ) $(COBJ)

clean :
	$(RM) $(AOBJ) $(COBJ)

$(COBJ): %.o: %.c include/acess.h
	@echo === GCC -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(AOBJ): %.ao: %.asm
	@echo === NASM -c $< -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

debug.o:	include/reloc_elf.h
vfs.o fs_rom.o fs_fat.o fs_devfs2.o fs_internal.o:	include/vfs.h
drv_pci.o drv_ps2mouse.o drv_vbe.o drv_ide.o drv_fdd.o drv_tty.o:	include/vfs.h include/fs_devfs2.h
