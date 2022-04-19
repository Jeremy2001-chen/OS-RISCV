#include <MemoryConfig.h>

    .section .boot
    .globl _start
_start:
    la sp, kernelStack
    li t0, KERNEL_STACK_SIZE
    add sp, sp, t0
    call main

loop:
    j loop

    .section .stack
    .globl kernelStack
    .align 12
kernelStack:
    .space KERNEL_STACK_SIZE
    
    .section .pgdir
    .align 12
    .globl kernelPageDirectory
kernelPageDirectory:
    .space PAGE_SIZE

