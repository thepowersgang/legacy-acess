ENTRY(_start)
OUTPUT_FORMAT(binary)

SECTIONS {
    loadTo = 0x400000;
    entrypoint = _start;
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
   codeLength = . - loadTo;

   .bss : AT(ADDR(.bss)) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
        bss = .;
   }
   end = .;
   _end = .;
   magic = loadTo + entrypoint + codeLength;
}