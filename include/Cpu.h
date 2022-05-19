#ifndef _CPU_H_
#define _CPU_H_

struct Cpu {
    int interruptLayer; //depth of disable interrupt
    int lastInterruptEnable; //were interrupts enabled before disable interrupt
};

struct Cpu* myCpu();

#endif