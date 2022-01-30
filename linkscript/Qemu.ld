ENTRY(_start)

BASE_ADDRESS        =   0x80200000;

SECTIONS
{
    . = BASE_ADDRESS;

    kernelStart = .;

    . = ALIGN(PAGE_SIZE);

    textStart = .;
    .text : {
        *(.text .text.*)
        . = ALIGN(4096);
        _trampoline = .;
        *(trampsce)
        . = ALIGN(4096);
        ASSERT(. - _trampoline <= 4096, "error: trampoline larger than one page");
        PROVIDE(textEnd = .);
    }

    . = ALIGN(4096);
    rodataStart = .;
    .rodata : {
        *(.rodata .rodata.*)
        rodataEnd = .;
    }

    . = ALIGN(4096);
    dataStart = .;
    .data : {
        *(.data .data.*)
        dataEnd = .;
    }

    . = ALIGN(4096);
    bssStart = .;
    .bss : {
        *(.bss .bss.*)
        . = ALIGN(4096);
        _pgdir = .;
        *(.pgdir)
        ASSERT(. - _pgdir == 4096, "error: pgdir not one page");
        bssEnd = .;
    }

    . = ALIGN(4096);
    PROVIDE(kernelEnd = .);
}