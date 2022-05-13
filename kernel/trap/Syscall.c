#include <Syscall.h>
#include <Driver.h>
#include <Process.h>
#include <Type.h>
#include <Page.h>
#include <Riscv.h>
#include <Trap.h>
#include <Spinlock.h>
#include <Sysfile.h>
#include <exec.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_GET_PROCESS_ID]    syscallGetProcessId,
    [SYSCALL_SCHED_YIELD]       syscallYield,
    [SYSCALL_PROCESS_DESTORY]   syscallProcessDestory,
    [SYSCALL_CLONE]             syscallClone,
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
    [SYSCALL_OPENAT]            syscallOpenAt,
    [SYSCALL_GET_CPU_TIMES]     syscallGetCpuTimes,
    [SYSCALL_GET_TIME]          syscallGetTime,
    [SYSCALL_SLEEP_TIME]        syscallSleepTime,
    [SYSCALL_DUP3]              syscallDupAndSet,
    [SYSCALL_CHDIR]             syscallChangeDir,
    [SYSCALL_CWD]               syscallGetWorkDir,
    [SYSCALL_MKDIRAT]           syscallMakeDirAt,
    [SYSCALL_BRK]               syscallBrk,
    [SYSCALL_SBRK]              syscallSetBrk,
    [SYSCALL_FSTAT]             syscallGetFileState,
    [SYSCALL_MAP_MEMORY]        syscallMapMemory,
    [SYSCALL_UNMAP_MEMORY]      syscallUnMapMemory,
    [SYSCALL_READDIR]           syscallReadDirectory,
    [SYSCALL_EXEC]              syscallExec,
    [SYSCALL_CWD]               syscallCwd
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

//todo: support mode
void syscallWait() {
    Trapframe* trapframe = getHartTrapFrame();
    int pid = trapframe->a0;
    u64 addr = trapframe->a1;    
    trapframe->a0 = wait(pid, addr);
}

void syscallYield() {
    kernelProcessCpuTimeEnd();
	yield();
}

void syscallClone() {
    Trapframe *tf = getHartTrapFrame();
    processFork(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4);
}

void syscallDev() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = sys_dev();
}

void syscallExit() {
    Trapframe* trapframe = getHartTrapFrame();
    struct Process* process;
    int ret, ec = trapframe->a0;

    if ((ret = pid2Process(0, &process, 1)) < 0) {
        panic("Process exit error\n");
        return;
    }    

    process->retValue = (ec << 8); //todo
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

void syscallGetCpuTimes() {
    Trapframe *tf = getHartTrapFrame();
    // printf("addr : %lx\n", tf->a0);
    int cow;
    CpuTimes *ct = (CpuTimes*)vir2phy(myproc()->pgdir, tf->a0, &cow);
    if (cow) {
        cowHandler(myproc()->pgdir, tf->a0);
    }
    *ct = myproc()->cpuTime;
    tf->a0 = 19260817;
}

void syscallGetTime() {
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000;
    copyout(myproc()->pgdir, tf->a0, (char*)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

void syscallSleepTime() {
    Trapframe *tf = getHartTrapFrame();
    TimeSpec ts;
    copyin(myproc()->pgdir, (char*)&ts, tf->a0, sizeof(TimeSpec));
    myproc()->awakeTime = r_time() +  ts.second * 1000000 + ts.microSecond;
    kernelProcessCpuTimeEnd();
    yield();
}

void syscallGetWorkDir() {
    Trapframe *trapframe = getHartTrapFrame();
    trapframe->a0 = sys_cwd();
}

void syscallBrk() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 addr = trapframe->a0;
    if (addr == 0) {
        trapframe->a0 = myproc()->heapBottom;
    } else if (addr >= myproc()->heapBottom) {
        trapframe->a0 = (sys_sbrk(addr - myproc()->heapBottom) != -1);
    } else 
        trapframe->a0 = -1;
}

void syscallSetBrk() {
    Trapframe *trapframe = getHartTrapFrame();
    u32 len = trapframe->a0;
    trapframe->a0 = sys_sbrk(len);
}

void syscallMapMemory() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 start = trapframe->a0, len = trapframe->a1, perm = trapframe->a2;
    bool alloc = (start == 0);
    if (alloc) {
        myproc()->heapBottom = UP_ALIGN(myproc()->heapBottom, 12);
        start = myproc()->heapBottom;
        myproc()->heapBottom = UP_ALIGN(myproc()->heapBottom + len, 12); 
    }
    u64 addr = start, end = start + len;
    start = DOWN_ALIGN(start, 12);
    while (start < end) {
        u64* pte;
        u64 pa = pageLookup(myproc()->pgdir, start, &pte);
        if (pa > 0 && (*pte & PTE_COW)) {
            cowHandler(myproc()->pgdir, start);
        }
        PhysicalPage* page;
        if (pageAlloc(&page) < 0) {        
            trapframe->a0 = -1;
            return ;
        }
        pageInsert(myproc()->pgdir, start, page2pa(page), perm | PTE_USER);
        start += PGSIZE;
    }

    struct file* fd;
    if (argfd(4, 0, &fd)) {
        trapframe->a0 = -1;
        return ;
    }
    fd->off = trapframe->a5;
    if (fileread(fd, addr, len)) {
        trapframe->a0 = addr;
    } else {
        trapframe->a0 = -1;
    }
}

void syscallUnMapMemory() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 start = trapframe->a0, len = trapframe->a1, end = start + len;
    start = DOWN_ALIGN(start, 12);
    while (start < end) {
        if (pageRemove(myproc()->pgdir, start) < 0) {
            trapframe->a0 = -1;
            return ;
        }
        start += PGSIZE;
    }
    trapframe->a0 = 0;
}

void syscallReadDirectory() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = sys_readdir();
}

void syscallExec() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = sys_exec();
}

void syscallCwd() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = sys_cwd();
}