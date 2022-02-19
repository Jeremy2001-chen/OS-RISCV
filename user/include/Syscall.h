#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include "../../include/SyscallId.h"

inline void syscallPutchar(char c) {
    register long a0 asm ("a0") = (long) c;
    register long a7 asm ("a7") = SYSCALL_PUTCHAR;
    asm volatile ("ecall": "+r" (a0): "r" (a7):);
}

inline int syscallFork() {
    register long a7 asm("a7") = SYSCALL_FORK;
    asm volatile ("ecall": : "r"(a7):);
    register long a0 asm("a0");
    return a0;
}

#endif