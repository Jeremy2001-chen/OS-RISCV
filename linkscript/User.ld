ENTRY(_userStart)
SECTIONS {
    . = 0x00a00000;

	_text = .;			/* Text and read-only data */
	.text : {
		*(.text)
		*(.fixup)
		*(.gnu.warning)
	}

	. = ALIGN(4096);	
  	_etext = .;			/* End of text section */

	.data : {			/* Data */
		*(.data)
		*(.rodata)
		*(.rodata.*)
		*(.eh_frame)
		CONSTRUCTORS
	}

    . = ALIGN(4096);
	_edata = .;			/* End of data section */

  	__bss_start = .;		/* BSS */
  	.bss : {
		*(.bss)
	}

  	. = ALIGN(4096);  	
  	end = . ;
}