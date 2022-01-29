#include <Type.h>
#include <Driver.h>

void consoleInit() {
    // To do
}

inline void putchar(char c) {
    register u64 a0 asm ("a0") = (u64) c;
    register u64 a7 asm ("a7") = (u64) SBI_CONSOLE_PUTCHAR;
    asm volatile ("ecall" : "+r" (a0) : "r" (a7) : "memory");
}