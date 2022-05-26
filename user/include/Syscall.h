#ifndef _USER_SYSCALL_H_
#define _USER_SYSCALL_H_

#include "../../include/Type.h"

inline u64 inline msyscall(long n, u64 _a0, u64 _a1, u64 _a2, u64
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

#endif