#include <Riscv.h>
#include <Driver.h>
#include <Timer.h>

void kernelVector();
void trapInit() {
    w_stvec((u64)kernelVector);
    w_sscratch(r_sstatus() | SSTATUS_SIE | SSTATUS_SPIE);
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE);
    SBI_CALL_1(SBI_SET_TIMER, r_time() + INTERVAL);
    while (true);
}

void kernelTrap() {
    printf("aaaaaaaaaa\n");
    while (true) ;
}