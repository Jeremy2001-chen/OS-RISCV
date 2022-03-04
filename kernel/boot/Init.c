#include <Driver.h>
#include <Page.h>
#include <Trap.h>
#include <Process.h>

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x3));
}

void main(u64 hartId) {
    initHartId(hartId);

    consoleInit();
    printfInit();
    printf("Hello, risc-v!\nhartId: %ld \n\n", hartId);

    memoryInit();
    trapInit();
    processInit();
    PROCESS_CREATE_PRIORITY(user_ProcessA, 1);
    PROCESS_CREATE_PRIORITY(user_ProcessB, 1);

    yield();
}