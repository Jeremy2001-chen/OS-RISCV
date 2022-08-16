	.section .text
    .globl _start
_start:
	la sp, _stack
    li a0, 48
	li a7, 4
	ecall
	# jal main
	li a7, 93
	ecall

	.section .data
_stack:
	.space 4096