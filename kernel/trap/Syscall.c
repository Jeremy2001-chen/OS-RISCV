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
    [SYSCALL_EXIT]              syscallExit,
    [SYSCALL_PIPE2]             syscallPipe,
    [SYSCALL_WRITE]             syscallWrite,
    [SYSCALL_READ]              syscallRead,
    [SYSCALL_CLOSE]             syscallClose,
    [SYSCALL_OPENAT]            syscallOpenat,
    [SYSCALL_OPEN]              syscallOpen,
    [SYSCALL_GET_CPU_TIMES]     syscallGetCpuTimes,
    [SYSCALL_GET_TIME]          syscallGetTime,
    [SYSCALL_SLEEP_TIME]        syscallSleepTime,
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

void syscallPipe() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_pipe();
}

void syscallWrite() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_write();
}

void syscallRead() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_read();
}

void syscallClose() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_close();    
}

void syscallOpen() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_open();       
}

void syscallOpenat() {
    //todo       
    panic("syscall Openat not implement\n");
}

void syscallGetCpuTimes() {
    Trapframe *tf = getHartTrapFrame();
    // printf("addr : %lx\n", tf->a0);
    int cow;
    CpuTimes *ct = (CpuTimes*)vir2phy(myproc()->pgdir, tf->a0, &cow);
    if (cow) {
        cowHandler(myproc()->pgdir, tf->a0);
    }
    *ct = myproc()->cpuTime;
    tf->a0 = r_cycle();
}

void syscallGetTime() {
    Trapframe *tf = getHartTrapFrame();
    int cow;
    TimeSpec *ts = (TimeSpec*)vir2phy(myproc()->pgdir, tf->a0, &cow);
    if (cow) {
        cowHandler(myproc()->pgdir, tf->a0);
    }
    u64 time = r_time();
    ts->second = time / 1000000;
    ts->microSecond = time % 1000000;
    tf->a0 = 0;
}

void syscallSleepTime() {
    Trapframe *tf = getHartTrapFrame();
    TimeSpec *ts = (TimeSpec*)vir2phy(myproc()->pgdir, tf->a0, NULL);
    myproc()->awakeTime = r_time() +  ts->second * 1000000 + ts->microSecond;
    yield();
}