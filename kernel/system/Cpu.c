#include "Cpu.h"
#include "Riscv.h"

struct Cpu cpus[CPU_NUM];

inline struct Cpu* myCpu() {
    int r = r_hartid();
    return &cpus[r];
}