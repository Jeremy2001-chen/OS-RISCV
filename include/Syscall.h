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
void syscallBrk();
void syscallSetBrk();
void syscallMapMemory();
void syscallUnMapMemory();
void syscallExec();
void syscallUname();
void syscallSetTidAddress();
void syscallExitGroup();
void syscallSignProccessMask();
void syscallSignalAction();
void syscallSignalTimedWait();
void syscallGetTheardId();
void syscallProcessResourceLimit();
void syscallIOControl();
void syscallSocket();
void syscallBind();
void syscallGetSocketName();
void syscallSetSocketOption();
void syscallSendTo();
void syscallReceiveFrom();
void syscallFcntl();
void syscallListen();
void syscallConnect();
void syscallAccept();
void syscallFutex();
void syscallThreadKill();

extern void (*syscallVector[])(void);

#endif