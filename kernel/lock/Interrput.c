#include "Interrupt.h"
#include "Riscv.h"
#include "Cpu.h"
#include "Driver.h"

void interruptPush(void) {
    int oldInterruptEnable = intr_get();
    intr_off();

    struct Cpu* cpu = myCpu();
    if (cpu->interruptLayer == 0)
        cpu->lastInterruptEnable = oldInterruptEnable;
    cpu->interruptLayer++;
}

void interruptPop(void) {
    struct Cpu* cpu = myCpu();
    if (intr_get()) {
        panic("Interrupt bit still have!\n");
    }

    if (cpu->interruptLayer < 0) {
        panic("Interrupt close error! Not match!\n");
    }

    cpu->interruptLayer--;
    if (cpu->interruptLayer == 0 && cpu->lastInterruptEnable)
        intr_on();
}