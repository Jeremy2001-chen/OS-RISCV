#ifndef _HART_H_
#define _HART_H_
#include "Process.h"

struct Hart {
    int interruptLayer; //depth of disable interrupt
    int lastInterruptEnable; //were interrupts enabled before disable interrupt
};

struct Hart* myHart(void);
Trapframe* getHartTrapFrame(void);
u64 getHartKernelTopSp();

#endif