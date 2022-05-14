#ifndef _USER_SYSCALL_LIB_H_
#define _USER_SYSCALL_LIB_H_

#include "../../include/SyscallId.h"
#include "../../include/Type.h"
#include "./include/Syscall.h"
#include "./include/userfile.h"

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
    return msyscall(SYSCALL_CLONE, 17, 0, 0, 0, 0, 0);
}

static inline u32 getpid() {
    return msyscall(SYSCALL_GET_PID, 0, 0, 0, 0, 0, 0);
}

static inline u32 getppid() {
    return msyscall(SYSCALL_GET_PARENT_PID, 0, 0, 0, 0, 0, 0);
}

static inline int wait(u64 addr) {
    return msyscall(SYSCALL_WAIT, (u64)-1, addr, 0, 0, 0, 0);
}

static inline int waitpid(int pid, u64 addr, int mode) {
    return msyscall(SYSCALL_WAIT, (u64)pid, addr, (u64)mode, 0, 0, 0);
}


static inline int dev(int fd, int mode) {
    return msyscall(SYSCALL_DEV, (u64)fd, (u64)mode, 0, 0, 0, 0);
}

static inline int write(int fd, const void* src, int len) {
    return msyscall(SYSCALL_WRITE, (u64)fd, (u64)src, (u64)len, 0, 0, 0);
}

static inline int dup(int fd) {
    return msyscall(SYSCALL_DUP, (u64)fd, 0, 0, 0, 0, 0);
}

static inline int exit(int ec) {
    return msyscall(SYSCALL_EXIT, (u64)ec, 0, 0, 0, 0, 0);
}

static inline int pipe(int fd[2]) {
    return msyscall(SYSCALL_PIPE2, (u64)fd, 0, 0, 0, 0, 0);
}

static inline int close(int fd) {
    return msyscall(SYSCALL_CLOSE, (u64)fd, 0, 0, 0, 0, 0);
}

static inline int open(const char* path, int flags) {
    return msyscall(SYSCALL_OPENAT, AT_FDCWD, (u64)path, (u64)flags, O_RDWR, 0, 0);
}

static inline int mkdir(const char* path, int mode) {
    return msyscall(SYSCALL_MKDIRAT, AT_FDCWD, (u64)path, (u64)mode, 0, 0, 0);
}

static inline int getdirent(int fd, char * buf, int n) {
    return msyscall(SYSCALL_GET_DIRENT, (u64)fd, (u64)buf, (u64)n, 0, 0, 0);
}
#endif