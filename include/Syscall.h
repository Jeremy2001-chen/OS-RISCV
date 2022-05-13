#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include <SyscallId.h>

void syscallPutchar();
void syscallGetProcessId();
void syscallYield();
void syscallProcessDestory();
void syscallClone();
void syscallPutString();
void syscallGetProcessId();
void syscallGetParentProcessId();
void syscallWait();
void syscallExit();
void syscallGetCpuTimes();
void syscallGetTime();
void syscallSleepTime();
void syscallChdir();
void syscallGetWorkDir();
void syscallMakeDir();
void syscallBrk();
void syscallSetBrk();
void syscallMapMemory();
void syscallUnMapMemory();
void syscallExec();

extern void (*syscallVector[])(void);

#endif