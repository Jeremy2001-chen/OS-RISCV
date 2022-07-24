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
#include <Signal.h>
#include <Socket.h>
#include <Mmap.h>
#include <Futex.h>
#include <Thread.h>
#include <Clone.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_SCHED_YIELD]       syscallYield,
    [SYSCALL_CLONE]             syscallClone,
    [SYSCALL_PUT_STRING]        syscallPutString,
    [SYSCALL_GET_PID]           syscallGetProcessId,
    [SYSCALL_GET_PARENT_PID]    syscallGetParentProcessId,
    [SYSCALL_WAIT]              syscallWait,
    [SYSCALL_DEV]               syscallDevice,
    [SYSCALL_DUP]               syscallDup,
    [SYSCALL_EXIT]              syscallExit,
    [SYSCALL_PIPE2]             syscallPipe,
    [SYSCALL_WRITE]             syscallWrite,
    [SYSCALL_READ]              syscallRead,
    [SYSCALL_CLOSE]             syscallClose,
    [SYSCALL_OPENAT]            syscallOpenAt,
    [SYSCALL_GET_CPU_TIMES]     syscallGetCpuTimes,
    [SYSCALL_GET_TIME_OF_DAY]   syscallGetTime,
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
    [SYSCALL_READDIR]           syscallReadDir,
    [SYSCALL_EXEC]              syscallExec,
    [SYSCALL_GET_DIRENT]        syscallGetDirent,
    [SYSCALL_MOUNT]             syscallMount,
    [SYSCALL_UMOUNT]            syscallUmount,
    [SYSCALL_LINKAT]            syscallLinkAt,
    [SYSCALL_UNLINKAT]          syscallUnlinkAt,
    [SYSCALL_UNAME]             syscallUname,
    [SYSCALL_SET_TID_ADDRESS]   syscallSetTidAddress,
    [SYSCALL_EXIT_GROUP]        syscallExitGroup,
    [SYSCALL_SIGNAL_PROCESS_MASK] syscallSignProccessMask,
    [SYSCALL_SIGNAL_ACTION] syscallSignalAction,
    [SYSCALL_SIGNAL_TIMED_WAIT] syscallSignalTimedWait,
    [SYSCALL_GET_THREAD_ID] syscallGetTheardId,
    [SYSCALL_PROCESS_RESOURSE_LIMIT] syscallProcessResourceLimit,
    [SYSCALL_GET_TIME] syscallGetTime,
    [SYSCALL_LSEEK] syscallLSeek,
    [SYSCALL_IOCONTROL] syscallIOControl,
    [SYSCALL_SOCKET] syscallSocket,
    [SYSCALL_BIND] syscallBind,
    [SYSCALL_GET_SOCKET_NAME] syscallGetSocketName,
    [SYSCALL_SET_SOCKET_OPTION] syscallSetSocketOption,
    [SYSCALL_SEND_TO] syscallSendTo,
    [SYSCALL_RECEIVE_FROM] syscallReceiveFrom,
    [SYSCALL_FCNTL] syscallFcntl,
    [SYSCALL_LISTEN] syscallListen,
    [SYSCALL_CONNECT] syscallConnect,
    [SYSCALL_ACCEPT] syscallAccept,
    [SYSCALL_WRITE_VECTOR] syscallWriteVector,
    [SYSCALL_FUTEX] syscallFutex,
    [SYSCALL_THREAD_KILL] syscallThreadKill,
    [SYSCALL_POLL] syscallPoll,
    [SYSCALL_MEMORY_PROTECT] syscallMemoryProtect
};

extern struct Spinlock printLock;

void syscallPutchar() {
    Trapframe* trapframe = getHartTrapFrame();
    acquireLock(&printLock);
    putchar(trapframe->a0);
    releaseLock(&printLock);
}

void syscallGetProcessId() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = myProcess()->processId;
}

void syscallGetParentProcessId() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = myProcess()->parentId;
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
    tf->a0 = clone(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4);
}

void syscallExit() {
    Trapframe* trapframe = getHartTrapFrame();
    Thread* th; 
    int ret, ec = trapframe->a0;

    if ((ret = tid2Thread(0, &th, 1)) < 0) {
        panic("Process exit error\n");
        return;
    }    

    th->retValue = (ec << 8); //todo
    threadDestroy(th);
    //will not reach here
    panic("sycall exit error");
}

void syscallPutString() {
    Trapframe* trapframe = getHartTrapFrame();
    //printf("hart %d, env %lx printf string:\n", hartId, currentProcess[hartId]->id);
    u64 va = trapframe->a0;
    int len = trapframe->a1;
    u64* pte;
    u64 pa = pageLookup(myProcess()->pgdir, va, &pte) + (va & 0xfff);
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

void syscallGetCpuTimes() {
    Trapframe *tf = getHartTrapFrame();
    // printf("addr : %lx\n", tf->a0);
    Process *p = myProcess();
    copyout(p->pgdir, tf->a0, (char*)&p->cpuTime, sizeof(CpuTimes));
    tf->a0 = (r_cycle() & 0x3FFFFFFF);
}

void syscallGetTime() {
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000;
    copyout(myProcess()->pgdir, tf->a0, (char*)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

void syscallSleepTime() {
    Trapframe *tf = getHartTrapFrame();
    TimeSpec ts;
    copyin(myProcess()->pgdir, (char*)&ts, tf->a0, sizeof(TimeSpec));
    myThread()->awakeTime = r_time() +  ts.second * 1000000 + ts.microSecond;
    kernelProcessCpuTimeEnd();
    yield();
}

void syscallBrk() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 addr = trapframe->a0;
    if (addr == 0) {
        trapframe->a0 = myProcess()->heapBottom;
    } else if (addr >= myProcess()->heapBottom) {
        trapframe->a0 = (sys_sbrk(addr - myProcess()->heapBottom) != -1);
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
    u64 start = trapframe->a0, len = trapframe->a1, perm = trapframe->a2, flags = trapframe->a3;
    printf("mmap: %lx %lx %lx %lx\n", start, len, perm, flags);
    bool alloc = (start == 0);
    if (alloc) {
        myProcess()->heapBottom = UP_ALIGN(myProcess()->heapBottom, 12);
        start = myProcess()->heapBottom;
        myProcess()->heapBottom = UP_ALIGN(myProcess()->heapBottom + len, 12); 
    }
    u64 addr = start, end = start + len;
    start = DOWN_ALIGN(start, 12);
    while (start < end) {
        u64* pte;
        u64 pa = pageLookup(myProcess()->pgdir, start, &pte);
        if (pa > 0 && (*pte & PTE_COW)) {
            cowHandler(myProcess()->pgdir, start);
        }
        PhysicalPage* page;
        if (pageAlloc(&page) < 0) {        
            trapframe->a0 = -1;
            return ;
        }
        pageInsert(myProcess()->pgdir, start, page2pa(page), perm | PTE_USER | PTE_READ | PTE_WRITE);
        start += PGSIZE;
    }

    struct File* fd;
    if (flags & MAP_ANONYMOUS) {
        trapframe->a0 = addr;
        return;
    }

    if (argfd(4, 0, &fd)) {
        // printf("fd: %x\n", trapframe->a4);
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
        if (pageRemove(myProcess()->pgdir, start) < 0) {
            trapframe->a0 = -1;
            return ;
        }
        start += PGSIZE;
    }
    trapframe->a0 = 0;
}

void syscallExec() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = sys_exec();
}

void syscallUname() {
    struct utsname {
	    char sysname[65];
	    char nodename[65];
	    char release[65];
	    char version[65];
	    char machine[65];
	    char domainname[65];
    } uname;
    strncpy(uname.sysname, "my_linux", 65);
    strncpy(uname.nodename, "my_node", 65);
    strncpy(uname.release, "MIPS-OS", 65);
    strncpy(uname.version, "0.1.0", 65);
    strncpy(uname.machine, "Risc-V sifive_u", 65);
    strncpy(uname.domainname, "Beijing", 65);
    Trapframe *tf = getHartTrapFrame();
    copyout(myProcess()->pgdir, tf->a0, (char*)&uname, sizeof(struct utsname));
}

void syscallSetTidAddress() {
    Trapframe *tf = getHartTrapFrame();
    // copyout(myProcess()->pgdir, tf->a0, (char*)(&myProcess()->id), sizeof(u64));
    myThread()->clearChildTid = tf->a0;
    tf->a0 = myThread()->id;
}

void syscallExitGroup() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallSignProccessMask() {
    Trapframe *tf = getHartTrapFrame();
    u64 how = tf->a0;
    SignalSet set;
    Process *p = myProcess();
    copyin(p->pgdir, (char*)&set, tf->a1, sizeof(SignalSet));
    if (tf->a2 != 0) {
        copyout(p->pgdir, tf->a2, (char*)(&myThread()->blocked), sizeof(SignalSet));
    }
    tf->a0 = signProccessMask(how, &set);
}

void syscallSignalAction() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = doSignalAction(tf->a0, tf->a1, tf->a2);
}

void syscallSignalTimedWait() {
    TimeSpec ts;
    Trapframe *tf = getHartTrapFrame();
    Process *p = myProcess();
    if (tf->a2) {
        copyin(p->pgdir, (char*) &ts, tf->a2, sizeof(TimeSpec));
    }
    SignalSet signalSet;
    copyin(p->pgdir, (char*) &signalSet, tf->a0, sizeof(SignalSet));
    SignalInfo info;
    copyin(p->pgdir, (char*) &info, tf->a0, sizeof(SignalInfo));
    tf->a0 = doSignalTimedWait(&signalSet, &info, tf->a2 ? &ts: 0);
}

void syscallGetTheardId() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = myThread()->id;
}

void syscallProcessResourceLimit() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallIOControl() {
    Trapframe *tf = getHartTrapFrame();
    printf("fd: %d, cmd: %d, argc: %d\n", tf->a0, tf->a1, tf->a2);
    tf->a0 = 0;
}

void syscallSocket() {
    Trapframe *tf = getHartTrapFrame();
    int domain = tf->a0, type = tf->a1, protocal = tf->a2;
    tf->a0 = createSocket(domain, type, protocal);
}

void syscallBind() {
    Trapframe *tf = getHartTrapFrame();
    assert(tf->a2 == sizeof(SocketAddr));
    SocketAddr sa;
    copyin(myProcess()->pgdir, (char*)&sa, tf->a1, tf->a2);
    tf->a0 = bindSocket(tf->a0, &sa);
}

void syscallGetSocketName() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = getSocketName(tf->a0, tf->a1);
}

void syscallSetSocketOption() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallSendTo() {
    static char buf[PAGE_SIZE];
    Trapframe *tf = getHartTrapFrame();
    assert(tf->a5 == sizeof(SocketAddr));
    SocketAddr sa;
    copyin(myProcess()->pgdir, (char *)&sa, tf->a4, sizeof(SocketAddr));
    u32 len = MIN(tf->a2, PAGE_SIZE);
    copyin(myProcess()->pgdir, buf, tf->a1, len);
    tf->a0 = sendTo(tf->a0, buf, tf->a2, tf->a3, &sa);
}

void syscallReceiveFrom() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = receiveFrom(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4);
}

void syscallFcntl() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = -1;
}

void syscallListen() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallConnect() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallAccept() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallFutex() {
    Trapframe *tf = getHartTrapFrame();
    int op = tf->a1, val = tf->a2, userVal;
    u64 uaddr = tf->a0;
    printf("addr: %lx, op: %d, val: %d\n", uaddr, op, val);
    op &= (FUTEX_PRIVATE_FLAG - 1);
    switch (op)
    {
        case FUTEX_WAIT:
            copyin(myProcess()->pgdir, (char*)&userVal, uaddr, sizeof(int));
            printf("val: %d\n", userVal);
            if (userVal != val) {
                tf->a0 = -1;
                return;
            }
            futexWait(uaddr, myThread());
            break;
        case FUTEX_WAKE:
            printf("val: %d\n", val);
            futexWake(uaddr, val);
            break;
        default:
            panic("Futex type not support!\n");
    }
    tf->a0 = 0;
}

void syscallThreadKill() {
    Trapframe *tf = getHartTrapFrame();
    int tid = tf->a0, signal = tf->a1;
    Thread* th;
    int r = tid2Thread(tid, &th, 0);
    if (r < 0) {
        tf->a0 = r;
        panic("Can't find thread %lx\n", tid);
        return;
    }
    th->pending |= (1ul<<signal);
    printf("tid: %lx, pending: %lx sign: %d\n", tid, th->pending, signal);
    tf->a0 = 0;
}

void syscallPoll() {
    Trapframe *tf = getHartTrapFrame();
    struct pollfd {
        int fd;
        short events;
        short revents;
    };
    struct pollfd p;
    u64 startva = 0;
    int n = tf->a1;
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        copyin(myProcess()->pgdir, (char*)&p, startva, sizeof(struct pollfd));
        p.revents = 0;
        copyout(myProcess()->pgdir, startva, (char*)&p, sizeof(struct pollfd));
        startva += sizeof(struct pollfd);
        cnt += p.revents != 0;
    }
    tf->a0 = cnt;
}

void syscallMemoryProtect() {
    Trapframe *tf = getHartTrapFrame();
    printf("mprotect va: %lx, length: %lx\n", tf->a0, tf->a1);
    tf->a0 = 0;
}