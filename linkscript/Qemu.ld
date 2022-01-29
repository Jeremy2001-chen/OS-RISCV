ENTRY(_start)

BASE_ADDRESS        =   0x80200000;

SECTIONS
{
    . = BASE_ADDRESS;

    kernel_start = .;

    . = ALIGN(4096);

    text_start = .;
    .text : {
        *(.text .text.*)
        . = ALIGN(4096);
        _trampoline = .;
        *(trampsce)
        . = ALIGN(4096);
        ASSERT(. - _trampoline <= 4096, "error: trampoline larger than one page");
        PROVIDE(text_end = .);
    }

    . = ALIGN(4096);
    rodata_start = .;
    .rodata : {
        *(.rodata .rodata.*)
        rodata_end = .;
    }

    . = ALIGN(4096);
    data_start = .;
    .data : {
        *(.data .data.*)
        data_end = .;
    }

    . = ALIGN(4096);
    bss_start = .;
    .bss : {
        *(.bss .bss.*)
        bss_end = .;
    }

    . = ALIGN(4096);
    PROVIDE(kernel_end = .);
}