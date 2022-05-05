#include <Syscall.h>
#include <Driver.h>
#include <Process.h>
#include <Type.h>
#include <Page.h>
#include <Riscv.h>
#include <Trap.h>
#include <Spinlock.h>
#include <sysfile.h>
#include <exec.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_GET_PROCESS_ID]    syscallGetProcessId,
    [SYSCALL_SCHED_YIELD]       syscallYield,
    [SYSCALL_PROCESS_DESTORY]   syscallProcessDestory,
    [SYSCALL_FORK]              syscallFork,
    [SYSCALL_PUT_STRING]        syscallPutString,
    [SYSCALL_GET_PID]           syscallGetProcessId,
    [SYSCALL_GET_PARENT_PID]    syscallGetParentProcessId,
    [SYSCALL_WAIT]              syscallWait,
    [SYSCALL_DEV]               syscallDev,
    [SYSCALL_DUP]               syscallDup,
    [SYSCALL_EXIT]              syscallExit
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

//todo not all finish!
void syscallWait() {
    Trapframe* trapframe = getHartTrapFrame();
    u64 addr = trapframe->a2;    
    trapframe->a0 = wait(addr);
}

void syscallYield() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = 0;
	yield();
}

void syscallFork() {
    processFork();
}

void syscallDev() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_dev();
}

void syscallDup() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_dup();
}

void syscallExit() {
    Trapframe* trapframe = getHartTrapFrame();
    struct Process* process;
    int ret, ec = trapframe->a1;

    if ((ret = pid2Process(0, &process, 1)) < 0) {
        panic("Process exit error\n");
        return;
    }    

    process->retValue = ec;
    processDestory(process);
    //will not reach here
    panic("sycall exit error");
}

void syscallPutString() {
    Trapframe* trapframe = getHartTrapFrame();
    int hartId = r_hartid();
    //printf("hart %d, env %lx printf string:\n", hartId, currentProcess[hartId]->id);
    u64 va = trapframe->a0;
    int len = trapframe->a1;
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    u64* pte;
    u64 pa = pageLookup(currentProcess[hartId]->pgdir, va, &pte) + (va & 0xfff);
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