#include <MemoryConfig.h>

    .section .text
    .globl _entry
_entry:
    li sp, KERNEL_STACK_TOP
    call main

loop:
    j loop
    
    .section .pgdir
    .align 12
    .globl kernelPageDirectory
kernelPageDirectory:
    .space PAGE_SIZE
