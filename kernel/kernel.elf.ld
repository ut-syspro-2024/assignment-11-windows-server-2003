OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(start)

SECTIONS {
	. = 0x100000000;

	.text : {
		*(.text)
	}

	. = ALIGN(4096);
	.rodata : {
		*(.rodata)
	}

	. = ALIGN(4096);
	.data : {
		*(.data)
	}

	. = ALIGN(4096);
	.bss : {
		__bss_start = .;
		*(.bss .bss.*)
		__bss_end = .;
	}

	/DISCARD/ : {
		*(.comment .comment.*)
	}
}
