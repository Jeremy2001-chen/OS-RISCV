#ifndef _CPU_H_
#define _CPU_H_

struct Cpu {
    int interruptLayer; //depth of disable interrupt
    int lastInterruptEnable; //were interrupts enabled before disable interrupt
};

#define CPU_NUM 5

struct Cpu* myCpu();

#endif