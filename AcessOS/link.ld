lstart = start - 0xC0000000;
ENTRY(lstart)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
   /* The kernel will live at 3GB + 1MB in the virtual
      address space, which will be mapped to 1MB in the
      physical address space. */
   . = 0xC0100000;

   .text : AT(ADDR(.text) - 0xC0000000) {
	   code = . - 0xC0000000;
       *(.text)
   }
   
   .utext ALIGN(0x1000) : AT(ADDR(.utext) - 0xC0000000) {
       *(.utext)
   }
   
   _gKernelCodeEnd = .;

   .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000) {
       *(.rodata*)
   }

   _User_ReturnExit = 0xFFFFF000 + _rUser_ReturnExit - _gUserCode;
   
   .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
	  data = . - 0xC0000000;
      *(.data)
      _gKernelSymbolTable = .;
	  *(.symtab)
      _gKernelStringTable = .;
	  *(.strtab)
   }
   .rdata ALIGN (0x1000) : AT(ADDR(.rdata) - 0xC0000000) {
       *(.rdata)
   }

   .bss : AT(ADDR(.bss) - 0xC0000000) {
       _sbss = .;
       *(COMMON)
       *(.bss)
       _ebss = .;
	   bss = . - 0xC0000000;
   }
   end = .;
   _gKernelEnd = (. - 0xC0000000 + 0xFFF)&0xFFFFF000;
}
