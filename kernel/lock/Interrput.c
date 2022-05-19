#include "Interrupt.h"
#include "Riscv.h"
#include "Hart.h"
#include "Driver.h"

void interruptPush(void) {
    int oldInterruptEnable = intr_get();
    intr_off();

    struct Hart* hart = myHart();
    if (hart->interruptLayer == 0)
        hart->lastInterruptEnable = oldInterruptEnable;
    hart->interruptLayer++;
}

void interruptPop(void) {
    struct Hart* hart = myHart();
    if (intr_get()) {
        panic("Interrupt bit still have!\n");
    }

    if (hart->interruptLayer < 0) {
        panic("Interrupt close error! Not match!\n");
    }

    hart->interruptLayer--;
    if (hart->interruptLayer == 0 && hart->lastInterruptEnable)
        intr_on();
}