    .section .text
    .globl _userStart
_userStart:
    addi a1, sp, 8
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