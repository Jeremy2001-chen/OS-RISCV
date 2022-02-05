#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <Type.h>
#include <Queue.h>

typedef struct Trapframe {
    u64 kenelSatp;
    u64 kernelSp;
    u64 kernelTrap;
    u64 epc;
} Trapframe;

enum ProcessState {
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

typedef struct Process {
    Trapframe trapframe;
    enum ProcessState state;
} Process;

Process* currentProcess();
void wakeup(void *channel);
void yield();

#endif