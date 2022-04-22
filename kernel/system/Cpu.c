#include "Cpu.h"
#include "Riscv.h"

struct Cpu cpus[HART_TOTAL_NUMBER];

inline struct Cpu* myCpu() {
    int r = r_hartid();
    return &cpus[r];
}