ENTRY(start)
OUTPUT_FORMAT(i686-pc-elf)

SECTIONS {
	. = 0x0;
	entrypoint = start;
   
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
}