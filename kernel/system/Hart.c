#include "Hart.h"
#include "Riscv.h"
#include "Process.h"
#include "MemoryConfig.h"

struct Hart harts[HART_TOTAL_NUMBER];

inline struct Hart* myHart() {
    int r = r_hartid();
    return &harts[r];
}

Trapframe* getHartTrapFrame() {
    return (Trapframe*)(TRAMPOLINE_BASE + PAGE_SIZE + r_hartid() * sizeof(Trapframe)); 
}

u64 getHartKernelTopSp() {
    extern char kernelStack[];
    return (u64)kernelStack + KERNEL_STACK_SIZE * (r_hartid() + 1);
}