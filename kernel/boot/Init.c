#include <Driver.h>
#include <Page.h>
#include <Trap.h>
#include <Process.h>
#include <Riscv.h>
#include <Sd.h>

volatile int mainCount = 1000;

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x7));
}

//void ffff(int);
u8 binary[1024];
void main(u64 hartId) {
    initHartId(hartId);

    if (mainCount == 1000) {
        extern u64 bssStart[];
        extern u64 bssEnd[];
        for (u64 *i = bssStart; i < bssEnd; i++) {
            *i = 0;
        }
        mainCount = mainCount + 1;

        consoleInit();
        printLockInit();
        pageLockInit();
        printf("Hello, risc-v!\nBoot hartId: %ld \n\n", hartId);

        memoryInit(hartId);
        /*sdInit();
        for (int j = 0; j < 10; j += 2) {
            for (int i = 0; i < 1024; i++) {
                binary[i] = i & 7;
            }
            sdWrite(binary, j, 2);
            for (int i = 0; i < 1024; i++) {
                binary[i] = 0;
            }
            sdRead(binary, j, 2);
            for (int i = 0; i < 1024; i++) {
                if (binary[i] != (i & 7)) {
                    panic("gg: %d ", j);
                    break;
                }
            }
            printf("finish %d\n", j);
        }*/

        for (int i = 1; i < 5; ++ i) {
            if (i != hartId) {
                unsigned long mask = 1 << i;
                setMode(i);
                sbi_send_ipi(&mask);
            }
        }
        //printf("end\n");
        __sync_synchronize();
        /*printf("reach end\n");*/
        mainCount++;
        
        trapInit();
        processInit();

        //PROCESS_CREATE_PRIORITY(ForkTest, 1);
        PROCESS_CREATE_PRIORITY(ProcessB, 3);

         yield();


    } else {
        __sync_synchronize();
        printf("Hello, risc-v!\nCurrent hartId: %ld \n\n", hartId);
        mainCount++;
        while (hartId != 4 || mainCount != 1005) {};

        startPage(hartId);
        trapInit();

        //PROCESS_CREATE_PRIORITY(ForkTest, 1);

        PROCESS_CREATE_PRIORITY(ProcessA, 2);
        //PROCESS_CREATE_PRIORITY(ProcessB, 3);

        yield();
    }
}