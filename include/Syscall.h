#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include <SyscallId.h>

void syscallPutchar();
void syscallYield();
void syscallProcessDestory();
void syscallClone();
void syscallPutString();
void syscallGetProcessId();
void syscallGetParentProcessId();
void syscallWait();
void syscallExit();
void syscallGetCpuTimes();
void syscallGetTimeOfDay();
void syscallGetClockTime();
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
void syscallProcessKill();
void syscallThreadKill();
void syscallThreadGroupKill();
void syscallPoll();
void syscallMemoryProtect();
void syscallGetRobustList();
void syscallSetRobustList();
void syscallStateFileSystem();
void syscallGetUserId();
void syscallGetEffectiveUserId();
void syscallMemoryBarrier();
void syscallSignalReturn();
void syscallLog();
void syscallSystemInfo();
void syscallGetResouceUsage();
void syscallSelect();
void syscallSetTimer();
void syscallMemorySychronize();

extern void (*syscallVector[])(void);

#endif