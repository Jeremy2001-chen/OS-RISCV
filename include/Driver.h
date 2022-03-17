#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <Type.h>
#include <assembly/Scause.h>

void consoleInit(void);
void printfInit(void);

#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

inline void putchar(char c) {
    register u64 a0 asm ("a0") = (u64) c;
    register u64 a7 asm ("a7") = (u64) SBI_CONSOLE_PUTCHAR;
    asm volatile ("ecall" : "+r" (a0) : "r" (a7) : "memory");
};

inline int getchar() {
    register u64 a7 asm ("a7") = (u64) SBI_CONSOLE_GETCHAR;
    register u64 a0 asm ("a0");
    asm volatile ("ecall" : "+r" (a0) : "r" (a7) : "memory");
    return a0;
}

void printf(const char *fmt, ...);
void _panic_(const char*, int, const char*, ...)__attribute__((noreturn));
#define panic(...) _panic_(__FILE__, __LINE__, __VA_ARGS__)

void consoleInterrupt(int);


#define SBI_CALL(which, arg0, arg1, arg2, arg3) ({		\
	register u64 a0 asm ("a0") = (u64)(arg0);	\
	register u64 a1 asm ("a1") = (u64)(arg1);	\
	register u64 a2 asm ("a2") = (u64)(arg2);	\
	register u64 a3 asm ("a3") = (u64)(arg3);	\
	register u64 a7 asm ("a7") = (u64)(which);	\
	asm volatile ("ecall"					\
		      : "+r" (a0)				\
		      : "r" (a1), "r" (a2), "r" (a3), "r" (a7)	\
		      : "memory");				\
})

/* Lazy implementations until SBI is finalized */
#define SBI_CALL_0(which) SBI_CALL(which, 0, 0, 0, 0)
#define SBI_CALL_1(which, arg0) SBI_CALL(which, arg0, 0, 0, 0)
#define SBI_CALL_2(which, arg0, arg1) SBI_CALL(which, arg0, arg1, 0, 0)
#define SBI_CALL_3(which, arg0, arg1, arg2) \
		SBI_CALL(which, arg0, arg1, arg2, 0)
#define SBI_CALL_4(which, arg0, arg1, arg2, arg3) \
		SBI_CALL(which, arg0, arg1, arg2, arg3)

static inline void sbi_send_ipi(const unsigned long* hart_mask) {
    SBI_CALL_1(SBI_SEND_IPI, hart_mask);
}

#endif