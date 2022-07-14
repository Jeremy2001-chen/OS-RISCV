    .section .text
    .globl _userStart
_userStart:
    addi a1, sp, 8
    ld	 a2, 0(sp)
    addi a2, a2, 1	
    sll	 a2, a2, 3
    add  a2, a1, a2
    call libMain
_userLoop:
    call _userLoop

    .section .buffer
    .align 12
    .globl printfBuffer
printfBuffer:
    .space 4 * 128

    .section .buffer
    .globl bufferLen
bufferLen:
    .word 0;