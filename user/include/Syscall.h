#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include "../../include/SyscallId.h"
#include "../../include/Type.h"

inline void syscallPutchar(char c) {
    register long a0 asm ("a0") = (long) c;
    register long a7 asm ("a7") = SYSCALL_PUTCHAR;
    asm volatile ("ecall": "+r"(a0): "r" (a7):);
}

inline long syscallFork() {
    register long a7 asm("a7") = SYSCALL_FORK;
    register long a0 asm("a0");
    asm volatile ("ecall": "+r"(a0): "r"(a7):);
    return a0;
}

inline void syscallPutString(char* buffer, u64 len) {
    register long a0 asm ("a0") = (long) buffer;
    register long a1 asm ("a1") = (long) len;
    register long a7 asm ("a7") = SYSCALL_PUT_STRING;
    asm volatile ("ecall": "+r"(a0), "+r"(a1): "r"(a7));
}

#endif