#include <Driver.h>
#include <Page.h>
#include <Trap.h>
#include <Process.h>
#include <Riscv.h>

static int mainCount = 0;

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x3));
}

void main(u64 hartId) {
    initHartId(hartId);

    if (mainCount == 0) {
        mainCount = mainCount + 1;

        consoleInit();
        printfInit();
        printf("Hello, risc-v!\nhartId: %ld \n\n", hartId);

        memoryInit();
        trapInit();
        processInit();

        PROCESS_CREATE_PRIORITY(ForkTest, 1);
//        PROCESS_CREATE_PRIORITY(ProcessB, 1);

        yield();
    } else {
        while(1);
    }
}