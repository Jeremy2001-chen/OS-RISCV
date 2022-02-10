    .section .text
    .globl _userStart
_userStart:
1:
    li a0, 100
    li a7, 1
    ecall
    beq a0, a0, 1b
    // call libmain