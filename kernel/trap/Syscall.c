#include <Syscall.h>
#include <Driver.h>
#include <Process.h>
#include <Type.h>
#include <Page.h>
#include <Riscv.h>
#include <Trap.h>
#include <Spinlock.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_GET_PROCESS_ID]    syscallGetProcessId,
    [SYS_SCHED_YIELD]           syscallYield,
    [SYSCALL_PROCESS_DESTORY]   syscallProcessDestory,
    [SYSCALL_FORK]              syscallFork,
    [SYSCALL_PUT_STRING]        syscallPutString,
    [SYS_GET_PID]               syscallGetProcessId,
    [SYS_GET_PARENT_PID]        syscallGetParentProcessId,

};

extern struct Spinlock printLock;
extern Process *currentProcess[HART_TOTAL_NUMBER];

void syscallPutchar() {
    Trapframe* trapframe = getHartTrapFrame();
    acquireLock(&printLock);
    putchar(trapframe->a0);
    releaseLock(&printLock);
}

void syscallGetProcessId() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = currentProcess[r_hartid()]->id;
}

void syscallGetParentProcessId() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = currentProcess[r_hartid()]->parentId;
}

void syscallProcessDestory() {
    Trapframe* trapframe = getHartTrapFrame();
    u32 processId = trapframe->a0;
    struct Process* process;
    int ret;

    if ((ret = pid2Process(processId, &process, 1)) < 0) {
        trapframe->a0 = ret;
        return;
    }    

    processDestory(process);
    trapframe->a0 = 0;
    return;
}

void syscallYield() {
	yield();
}

void syscallFork() {
    processFork();
}

void syscallPutString() {
    Trapframe* trapframe = getHartTrapFrame();
    u64 va = trapframe->a0;
    int len = trapframe->a1;
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    int hartId = r_hartid();
    u64* pte;
    u64 pa = pageLookup(currentProcess[hartId]->pgdir, va, &pte);
    if (pa == 0) {
        panic("Syscall put string address error!\nThe virtual address is %x, the length is %x\n", va, len);
    }
    char* start = (char*) pa;
    acquireLock(&printLock);
    while (len--) {
        putchar(*start);
        start++;
    }
    releaseLock(&printLock);
}