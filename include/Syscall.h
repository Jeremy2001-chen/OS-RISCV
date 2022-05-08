#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include <SyscallId.h>

void syscallPutchar();
void syscallGetProcessId();
void syscallYield();
void syscallProcessDestory();
void syscallFork();
void syscallPutString();
void syscallGetProcessId();
void syscallGetParentProcessId();
void syscallWait();
void syscallDev();
void syscallWrite();
void syscallDup();
void syscallExit();
void syscallPipe();
void syscallRead();
void syscallClose();
void syscallOpenat();
void syscallOpen();
void syscallGetCpuTimes();
void syscallGetTime();
void syscallSleepTime();
void syscallSetDup();
void syscallChdir();
void syscallGetWorkDir();
void syscallMakeDir();
void syscallBrk();
void syscallSetBrk();

extern void (*syscallVector[])(void);

#endif