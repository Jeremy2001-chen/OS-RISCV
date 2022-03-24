#include <Driver.h>
#include <Page.h>
#include <Trap.h>
#include <Process.h>
#include <Riscv.h>
#include <Sd.h>

static int mainCount = 1000;

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x7));
}

u8 binary[1024];
void main(u64 hartId) {
    initHartId(hartId);

    putchar('0');
    if (mainCount == 1000) {
        extern u64 bssStart[];
        extern u64 bssEnd[];
        for (u64 *i = bssStart; i < bssEnd; i++) {
            *i = 0;
        }
        mainCount = mainCount + 1;

        consoleInit();
        printfInit();
        printf("Hello, risc-v!\nBoot hartId: %ld \n\n", hartId);
        memoryInit();
        sdInit();
        for (int i = 0; i < 1024; i++) {
            binary[i] = i & 254;
        }
        sdWrite(binary, 1, 2);
        for (int i = 0; i < 1024; i++) {
            binary[i] = 0;
        }
        sdRead(binary, 1, 2);
        for (int i = 0; i < 1024; i++) {
            printf("%d ", binary[i]);
        }

        //trapInit();
        //processInit();
/*
        for (int i = 1; i < 5; ++ i)
            if (i != hartId) {
                unsigned long mask = 1 << i;
                sbi_send_ipi(&mask);
            }

        __sync_synchronize();
//        PROCESS_CREATE_PRIORITY(ForkTest, 1);
//        PROCESS_CREATE_PRIORITY(ProcessB, 1);

        printf("aaaa\n");
//        yield();*/
    } else {
        putchar('a' + hartId);
        while(1);
    }
}