    .section .text
    .globl _userStart
_userStart:
    call libMain

    .section .buffer
    .align 12
    .globl printfBuffer
printfBuffer:
    .space 4 * 128

    .section .buffer
    .globl bufferLen
bufferLen:
    .word 0;