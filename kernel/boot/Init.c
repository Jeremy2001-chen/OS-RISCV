#include <Driver.h>
#include <Page.h>
#include <Trap.h>
#include <Process.h>
#include <Riscv.h>
#include <Sd.h>
#include <fat.h>
#include <bio.h>
#include <file.h>
#include <Sysfile.h>
#include <Riscv.h>
#define SINGLE_CORE

volatile int mainCount = 1000;
volatile int initFinish = 0;

static inline void initHartId(u64 hartId) {
    asm volatile("mv tp, %0" : : "r" (hartId & 0x7));
}

extern struct superblock *fat;

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

        // while (getchar() != '\n');
        memoryInit();
        processInit();


        sdInit();
        binit();
        fileinit();
        

        for (int i = 1; i < 5; ++ i) {
            if (i != hartId) {
                unsigned long mask = 1 << i;
                setMode(i);
                sbi_send_ipi(&mask);
            }
        }

        trapInit();

        __sync_synchronize();     

#ifdef SINGLE_CORE
        initFinish = 0;
#else
        initFinish = 1;
#endif

        // PROCESS_CREATE_PRIORITY(ProcessA, 2);
        // PROCESS_CREATE_PRIORITY(ProcessB, 3);
        // PROCESS_CREATE_PRIORITY(ForkTest, 5);
        // PROCESS_CREATE_PRIORITY(ProcessIdTest, 4);
        // PROCESS_CREATE_PRIORITY(ls, 1);
        // PROCESS_CREATE_PRIORITY(SysfileTest, 1);
        // PROCESS_CREATE_PRIORITY(PipeTest, 1);
        // PROCESS_CREATE_PRIORITY(ExecTest,1);
        // PROCESS_CREATE_PRIORITY(ExecToLs, 1);
        PROCESS_CREATE_PRIORITY(SyscallTest, 1);
        // PROCESS_CREATE_PRIORITY(WaitTest, 1);

    } else {
        while (initFinish == 0);
        __sync_synchronize();

        printf("Hello, risc-v!\nCurrent hartId: %ld \n\n", hartId);

        startPage();
        trapInit();

        //PROCESS_CREATE_PRIORITY(ForkTest, 1);
        //PROCESS_CREATE_PRIORITY(ProcessB, 3);
        //printf("Reach this place\n");
    }

    yield();
}