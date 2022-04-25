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

inline void syscallPutString(const char* buffer, u64 len) {
    register long a0 asm ("a0") = (long) buffer;
    register long a1 asm ("a1") = (long) len;
    register long a7 asm ("a7") = SYSCALL_PUT_STRING;
    asm volatile ("ecall": "+r"(a0), "+r"(a1): "r"(a7));
}

inline int syscallOpen(const char *path, int omode) {
    register long a0 asm ("a0") = (long) path;
    register long a1 asm ("a1") = (long) omode;
    register long a7 asm ("a7") = SYSCALL_OPEN;
    asm volatile ("ecall": "+r"(a0): "r"(a7), "r"(a1), "r"(a0));
    return a0;

}

inline int syscallRead(int fd, char* buf, int n) {
    register long a0 asm("a0") = (long)fd;
    register long a1 asm("a1") = (long)buf;
    register long a2 asm("a2") = (long)n;
    register long a7 asm("a7") = SYSCALL_READ;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a0), "r"(a1), "r"(a2));
    return a0;
}

inline int syscallWrite(int fd, char* buf, int n) {
    register long a0 asm("a0") = (long)fd;
    register long a1 asm("a1") = (long)buf;
    register long a2 asm("a2") = (long)n;
    register long a7 asm("a7") = SYSCALL_WRITE;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a0), "r"(a1), "r"(a2));
    return a0;
}

inline int syscallClose(int fd) {
    register long a0 asm("a0") = (long)fd;
    register long a7 asm("a7") = SYSCALL_CLOSE;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a0));
    return a0;
}

#endif