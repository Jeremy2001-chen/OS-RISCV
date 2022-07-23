#ifndef _Thread_H_
#define _Thread_H_

#include <Signal.h>
#include <Process.h>
#include <fat.h>

typedef struct Thread {
    Trapframe trapframe;
    LIST_ENTRY(Thread) link;
    u64 awakeTime;
    u32 id;
    LIST_ENTRY(Thread) scheduleLink;
    enum ProcessState state;
    struct Spinlock lock;
    u64 chan;//wait Object
    u64 currentKernelSp;
    int reason;
    u64 retValue;
    SignalSet blocked;
    SignalSet pending;
    u64 setChildTid;
    u64 clearChildTid;
    struct Process* process;
} Thread;

LIST_HEAD(ThreadList, Thread);

Thread* myThread(); // Get current running thread in this hart
void threadFree(Thread *th);
u64 getThreadTopSp(Thread* th);
SignalAction *getSignalHandler(Thread* th);

void threadInit();
int mainThreadAlloc(Thread **new, u64 parentId);
int threadAlloc(Thread **new, Process* process, u64 userSp);
int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm);
void threadRun(Thread* thread);
void threadDestroy(Thread* thread);

#define NORMAL         0
#define KERNEL_GIVE_UP 1

#endif