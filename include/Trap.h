#ifndef _TRAP_H_
#define _TRAP_H_

#include <MemoryConfig.h>
#include <Riscv.h>
#include <Process.h>
#include <Thread.h>

void trapInit();
void kernelVector();
void userVector();
void userReturn();
void userTrapReturn();
void trapframeDump(Trapframe*);

inline static u32 interruptServed() {
    int hart = r_tp();
    #ifndef QEMU
    return *(u32*)PLIC_MCLAIM(hart);
    #else
    return *(u32*)PLIC_SCLAIM(hart);
    #endif
}

inline static void interruptCompleted(int irq) {
    int hart = r_tp();
    #ifndef QEMU
    *(u32*)PLIC_MCLAIM(hart) = irq;
    #else
    *(u32*)PLIC_SCLAIM(hart) = irq;
    #endif
}

#ifdef QEMU
#define UART_IRQ 10
#define DISK_IRQ 1
#else
#define UART_IRQ 33
#define DISK_IRQ 27
#endif

#endif