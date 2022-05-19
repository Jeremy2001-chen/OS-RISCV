#ifndef _HART_H_
#define _HART_H_

#include "Type.h"
struct Hart {
    int interruptLayer; //depth of disable interrupt
    int lastInterruptEnable; //were interrupts enabled before disable interrupt
};

struct Trapframe;
struct Hart* myHart(void);
struct Trapframe* getHartTrapFrame(void);
u64 getHartKernelTopSp();

#endif