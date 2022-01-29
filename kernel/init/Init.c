#include <Type.h>
#include <Driver.h>

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x1));
}

void main(u64 hartId, u64 dtb_pa) {
    initHartId(hartId);

    printf("Hello, risc-v\n");
}