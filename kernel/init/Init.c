
#include <Driver.h>
#include <Page.h>

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x1));
}

void main(u64 hartId) {
    initHartId(hartId);

    consoleInit();
    printfInit();
    printf("Hello, risc-v!\nhartId: %ld \n\n", hartId);

    memoryInit();
}