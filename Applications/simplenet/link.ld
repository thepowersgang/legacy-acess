ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    loadTo = 0x400000;
    /*entrypoint = start;*/
    . = loadTo;

   .text : AT(ADDR(.text)) {
        code = .;
        *(.text)
        *(.rodata*)
   }

   .data ALIGN (0x1000) : AT(ADDR(.data)) {
        data = .;
        *(.data)
   }

   .bss : AT(ADDR(.bss)) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
        bss = .;
   }
   end = .;
   codeLength = end - loadTo;
   /*magic = loadTo + entrypoint + codeLength;*/
}