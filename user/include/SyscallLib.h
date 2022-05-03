#ifndef _USER_SYSCALL_LIB_H_
#define _USER_SYSCALL_LIB_H_

#include "../../include/SyscallId.h"
#include "../../include/Type.h"
#include "./include/Syscall.h"

extern char printfBuffer[];
extern int bufferLen;

static inline void putchar(char c) {
    msyscall(SYSCALL_PUTCHAR, c, 0, 0, 0, 0, 0);
}

static inline void putString(char* buffer, u64 len) {
    msyscall(SYSCALL_PUT_STRING, (u64) buffer, (u64) len, 0, 0, 0, 0);
}

static inline void processDestory(u32 processId) {
    msyscall(SYSCALL_PROCESS_DESTORY, (u64) processId, 0, 0, 0, 0, 0);
}

static inline u32 fork() {
    if (bufferLen > 0) {
        putString(printfBuffer, bufferLen);
        bufferLen = 0;
    }
    return msyscall(SYSCALL_FORK, 0, 0, 0, 0, 0, 0);
}

static inline u32 getpid() {
    return msyscall(SYSCALL_GET_PID, 0, 0, 0, 0, 0, 0);
}

static inline u32 getppid() {
    return msyscall(SYSCALL_GET_PARENT_PID, 0, 0, 0, 0, 0, 0);
}

static inline int wait(u64 addr) {
    return msyscall(SYSCALL_WAIT, addr, 0, 0, 0, 0, 0);
}
#endif