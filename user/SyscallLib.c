#include "../../include/SyscallId.h"
#include "../../include/Type.h"
#include "./include/Syscall.h"

void putchar(char c) {
    msyscall(SYSCALL_PUTCHAR, c, 0, 0, 0, 0, 0);
}

void putString(char* buffer, u64 len) {
    msyscall(SYSCALL_PUT_STRING, (u64) buffer, (u64) len, 0, 0, 0, 0);
}

void processDestory(u32 processId) {
    msyscall(SYSCALL_PROCESS_DESTORY, (u64) processId, 0, 0, 0, 0, 0);
}

u32 fork() {
    return msyscall(SYSCALL_FORK, 0, 0, 0, 0, 0, 0);
}

u32 getpid() {
    return msyscall(SYSCALL_GET_PID, 0, 0, 0, 0, 0, 0);
}

u32 getppid() {
    return msyscall(SYSCALL_GET_PARENT_PID, 0, 0, 0, 0, 0, 0);
}
