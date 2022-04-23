#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include "../../include/SyscallId.h"
#include "../../include/Type.h"

inline u64 msyscall(long n, u64 _a0, u64 _a1, u64 _a2, u64
		_a3, u64 _a4, u64 _a5) {
	register u64 a0 asm("a0") = _a0;
	register u64 a1 asm("a1") = _a1;
	register u64 a2 asm("a2") = _a2;
	register u64 a3 asm("a3") = _a3;
	register u64 a4 asm("a4") = _a4;
	register u64 a5 asm("a5") = _a5;
	register long syscall_id asm("a7") = n;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"
			(a5), "r"(syscall_id));
	return a0;
}

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

inline void syscallProcessDestory(u32 processId) {
    register long a0 asm ("a0") = (long) processId;
    register long a7 asm ("a7") = SYSCALL_PROCESS_DESTORY;
    asm volatile ("ecall": "+r"(a0): "r"(a7):);
}

inline int getpid() {
    return msyscall(SYS_GET_PID, 0, 0, 0, 0, 0, 0);
}
#endif