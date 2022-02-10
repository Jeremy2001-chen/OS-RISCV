    .section .text
    .globl _userStart
_userStart:
    li a0, 100
    li a7, 1
    ecall
    // call libmain