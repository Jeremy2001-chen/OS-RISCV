#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include <SyscallId.h>

void syscallPutchar();
void syscallGetProcessId();
void syscallYield();
void syscallProcessDestroy();
void syscallFork();

extern void (*syscallVector[])(void);

#endif