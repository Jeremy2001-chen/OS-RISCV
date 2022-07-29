	.section .signalTrampoline
    .globl signalTrampoline
    .align 12
signalTrampoline:
    li a7, 139
    ecall