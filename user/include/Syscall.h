#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include "../../include/Syscall.h"

inline void syscallPutchar(char c) {
    register long a0 asm ("a0") = (long) c;
    register long a7 asm ("a7") = SYSCALL_PUTCHAR;
    asm volatile ("ecall": "+r" (a0): "r" (a7):);
}

#endif