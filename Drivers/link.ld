ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	loadTo = 0x0;
    entrypoint = start;
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
	
	.got : {
		ld_GOT = .;
		*(.got)
		*(.got.plt)
	}
	   
	end = .;
	   
	codeLength = end - loadTo;
	magic = ld_GOT + entrypoint + codeLength;
}