# AcessOS Basic C Library
# Makefile

CC = i686-pc-elf-gcc.exe
AS = nasm_32
RM = @rm -f
LD = i686-pc-elf-ld.exe
OBJDUMP = i686-pc-elf-objdump.exe

CFLAGS = -Wall -fPIC -nostdinc -fno-builtin -I./include -fleading-underscore
ASFLAGS = -felf

OBJ_LIBC = heap.o syscalls.ao stdlib.o stub.o

.PHONY:	all clean

all: ../libc.so.1 ../crt0.o $(OBJ_LIBC)

clean:
	$(RM) ../libc.so.1 ../crt0.o

# Core C Library
../libc.so.1: $(OBJ_LIBC)
#	$(CC) -shared -Wl,-soname,libc.so.1 -Wl,--defsym,__GLOBAL_OFFSET_TABLE_=_GLOBAL_OFFSET_TABLE_ $(OBJ_LIBC) -o $@ -Wl,Map,map.txt
	$(LD) -x --oformat elf32-i386 --defsym __GLOBAL_OFFSET_TABLE_=_GLOBAL_OFFSET_TABLE_ -shared -soname libc.so.1 $(OBJ_LIBC) -o $@ -Map map.txt -e _SoMain
	$(OBJDUMP) -d $@ > libc.so.1.dsm
	$(OBJDUMP) -x -r -R $@ > libc.so.1.dmp
	copy ..\libc.so.1 ..\libc.so
	..\cpy2dsk.bat ..\libc.so.1 libc.so.1

# C Runtime 0
../crt0.o: crt0.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(filter %.c, $(OBJ_LIBC)): %.o: %.c
	$(CC) $(CFLAGS) -DBUILD_SO -o $@ -c $<

$(filter %.ao, $(OBJ_LIBC)): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<
