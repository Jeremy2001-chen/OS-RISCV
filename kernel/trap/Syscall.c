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
#include <Resource.h>
#include <FileSystem.h>
#include <KernelLog.h>
#include <Error.h>
#include <Select.h>
#include <pipe.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]                   syscallPutchar,
    [SYSCALL_SCHED_YIELD]               syscallYield,
    [SYSCALL_CLONE]                     syscallClone,
    [SYSCALL_PUT_STRING]                syscallPutString,
    [SYSCALL_GET_PID]                   syscallGetProcessId,
    [SYSCALL_GET_PARENT_PID]            syscallGetParentProcessId,
    [SYSCALL_WAIT]                      syscallWait,
    [SYSCALL_DEV]                       syscallDevice,
    [SYSCALL_DUP]                       syscallDup,
    [SYSCALL_EXIT]                      syscallExit,
    [SYSCALL_PIPE2]                     syscallPipe,
    [SYSCALL_WRITE]                     syscallWrite,
    [SYSCALL_READ]                      syscallRead,
    [SYSCALL_CLOSE]                     syscallClose,
    [SYSCALL_OPENAT]                    syscallOpenAt,
    [SYSCALL_GET_CPU_TIMES]             syscallGetCpuTimes,
    [SYSCALL_GET_TIME_OF_DAY]           syscallGetTimeOfDay,
    [SYSCALL_SLEEP_TIME]                syscallSleepTime,
    [SYSCALL_DUP3]                      syscallDupAndSet,
    [SYSCALL_fcntl]                     syscall_fcntl,
    [SYSCALL_CHDIR]                     syscallChangeDir,
    [SYSCALL_CWD]                       syscallGetWorkDir,
    [SYSCALL_MKDIRAT]                   syscallMakeDirAt,
    [SYSCALL_BRK]                       syscallBrk,
    [SYSCALL_SBRK]                      syscallSetBrk,
    [SYSCALL_FSTAT]                     syscallGetFileState,
    [SYSCALL_FSTATAT]                   syscallGetFileStateAt,
    [SYSCALL_MAP_MEMORY]                syscallMapMemory,
    [SYSCALL_UNMAP_MEMORY]              syscallUnMapMemory,
    [SYSCALL_READDIR]                   syscallReadDir,
    [SYSCALL_EXEC]                      syscallExec,
    [SYSCALL_GET_DIRENT]                syscallGetDirent,
    [SYSCALL_MOUNT]                     syscallMount,
    [SYSCALL_UMOUNT]                    syscallUmount,
    [SYSCALL_LINKAT]                    syscallLinkAt,
    [SYSCALL_UNLINKAT]                  syscallUnlinkAt,
    [SYSCALL_UNAME]                     syscallUname,
    [SYSCALL_SET_TID_ADDRESS]           syscallSetTidAddress,
    [SYSCALL_EXIT_GROUP]                syscallExitGroup,
    [SYSCALL_SIGNAL_PROCESS_MASK]       syscallSignProccessMask,
    [SYSCALL_SIGNAL_ACTION]             syscallSignalAction,
    [SYSCALL_SIGNAL_TIMED_WAIT]         syscallSignalTimedWait,
    [SYSCALL_GET_THREAD_ID]             syscallGetTheardId,
    [SYSCALL_PROCESS_RESOURSE_LIMIT]    syscallProcessResourceLimit,
    [SYSCALL_GET_TIME]                  syscallGetClockTime,
    [SYSCALL_LSEEK]                     syscallLSeek,
    [SYSCALL_IOCONTROL]                 syscallIOControl,
    [SYSCALL_SOCKET]                    syscallSocket,
    [SYSCALL_BIND]                      syscallBind,
    [SYSCALL_GET_SOCKET_NAME]           syscallGetSocketName,
    [SYSCALL_SET_SOCKET_OPTION]         syscallSetSocketOption,
    [SYSCALL_SEND_TO]                   syscallSendTo,
    [SYSCALL_RECEIVE_FROM]              syscallReceiveFrom,
    [SYSCALL_LISTEN]                    syscallListen,
    [SYSCALL_CONNECT]                   syscallConnect,
    [SYSCALL_ACCEPT]                    syscallAccept,
    [SYSCALL_WRITE_VECTOR]              syscallWriteVector,
    [SYSCALL_READ_VECTOR]               syscallReadVector,
    [SYSCALL_FUTEX]                     syscallFutex,
    [SYSCALL_PROCESS_KILL]              syscallProcessKill,
    [SYSCALL_THREAD_KILL]               syscallThreadKill,
    [SYSCALL_THREAD_GROUP_KILL]         syscallThreadGroupKill,
    [SYSCALL_POLL]                      syscallPoll,
    [SYSCALL_MEMORY_PROTECT]            syscallMemoryProtect,
    [SYSCALL_SET_ROBUST_LIST]           syscallSetRobustList,
    [SYSCALL_GET_ROBUST_LIST]           syscallGetRobustList,
    [SYSCALL_STATE_FS]                  syscallStateFileSystem,
    [SYSCALL_PREAD]                     syscallPRead,
    [SYSCALL_UTIMENSAT]                 syscallUtimensat,
    [SYSCALL_GET_EFFECTIVE_USER_ID]     syscallGetUserId,
    [SYSCALL_GET_EFFECTIVE_USER_ID]     syscallGetEffectiveUserId,
    [SYSCALL_GET_USER_ID]               syscallGetUserId,
    [SYSCALL_GET_PROCESS_GROUP_ID]      syscallGetUserId,
    [SYSCALL_SET_PROCESS_GROUP_ID]      syscallGetUserId,
    [SYSCALL_MEMORY_BARRIER]            syscallMemoryBarrier,
    [SYSCALL_SIGNAL_RETURN]             syscallSignalReturn,
    [SYSCALL_SEND_FILE]                 syscallSendFile,
    [SYSCALL_LOG]                       syscallLog,
    [SYSCALL_ACCESS]                    syscallAccess,
    [SYSCALL_GET_SYSTEM_INFO]           syscallSystemInfo,
    [SYSCALL_RENAMEAT]                  syscallRenameAt,
    [SYSCALL_READLINKAT]                syscallReadLinkAt,
    [SYSCALL_GET_RESOURCE_USAGE]        syscallGetResouceUsage,
    [SYSCALL_SELECT]                    syscallSelect,
    [SYSCALL_SET_TIMER]                 syscallSetTimer,
    [SYSCALL_UMASK]                     syscallUmask,
    [SYSCALL_FSSYC]                     syscallFileSychornize,
    [SYSCALL_MSYNC]                     syscallMemorySychronize,
    [SYSCALL_OPEN]                      syscallOpen,
    [SYSCALL_MADVISE]                   syscallMemeryAdvise,
    [SYSCALL_GET_GROUP_ID]              syscallGetGroupId,
    [SYSCALL_GET_EFFECTIVE_GROUP_ID]    syscallGetEffectiveGroupId,
    [SYSCALL_GET_RANDOM]                syscallGetRandom,
    [SYSCALL_CHANGE_MODAT]              syscallChangeModAt
};

extern struct Spinlock printLock;

void syscallPutchar() {
    Trapframe* trapframe = getHartTrapFrame();
    // acquireLock(&printLock);
    putchar(trapframe->a0);
    // releaseLock(&printLock);
}

void syscallGetProcessId() {
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = myProcess()->processId;
}

void syscallGetParentProcessId() {
    // printf("%s %d\n", __FILE__, __LINE__);
    Trapframe* trapframe = getHartTrapFrame();
    trapframe->a0 = myProcess()->parentId;
    // printf("%s %d\n", __FILE__, __LINE__);
}

//todo: support mode
void syscallWait() {
    Trapframe* trapframe = getHartTrapFrame();
    int pid = trapframe->a0;
    u64 addr = trapframe->a1;    
    int flags = trapframe->a2;
    trapframe->a0 = wait(pid, addr, flags);
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
        panic("thread exit error\n");
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
    // acquireLock(&printLock);
    while (len--) {
        putchar(*start);
        start++;
    }
    // releaseLock(&printLock);
}

void syscallGetCpuTimes() {
    Trapframe *tf = getHartTrapFrame();
    // printf("addr : %lx\n", tf->a0);
    Process *p = myProcess();
    copyout(p->pgdir, tf->a0, (char*)&p->cpuTime, sizeof(CpuTimes));
    tf->a0 = (r_cycle() & 0x3FFFFFFF);
}

void syscallGetClockTime() {
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeSpec ts;
    ts.second = time / 1000000;
    ts.nanoSecond = time % 1000000 * 1000; //todo
    // printf("kernel time: %ld second: %ld ms: %ld\n", time, ts.second, ts.nanoSecond);
    copyout(myProcess()->pgdir, tf->a1, (char*)&ts, sizeof(TimeSpec));
    tf->a0 = 0;
}

void syscallGetTimeOfDay() {
    Trapframe *tf = getHartTrapFrame();
    u64 time = r_time();
    TimeVal ts;
    ts.second = time / 1000000;
    ts.microSecond = time % 1000000; //todo
    // printf("kernel time: %ld second: %ld ms: %ld\n", time, ts.second, ts.microSecond);
    copyout(myProcess()->pgdir, tf->a1, (char*)&ts, sizeof(TimeVal));
    tf->a0 = 0;
}

void syscallSleepTime() {
    Trapframe *tf = getHartTrapFrame();
    TimeSpec ts;
    copyin(myProcess()->pgdir, (char*)&ts, tf->a0, sizeof(TimeSpec));
    myThread()->awakeTime = r_time() +  ts.second * 1000000 + ts.nanoSecond / 1000;
    kernelProcessCpuTimeEnd();
    yield();
}

void syscallBrk() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 addr = trapframe->a0;
    // printf("addr: %lx, heapbottom: %lx\n", addr, myProcess()->brkHeapBottom);
    if (addr == 0) {
        trapframe->a0 = myProcess()->brkHeapBottom;
    } else if (addr >= myProcess()->brkHeapBottom) {
        sys_sbrk(addr - myProcess()->brkHeapBottom);
        // trapframe->a0 = myProcess()->heapBottom;
    } else 
        {}
        // trapframe->a0 = -1;
    // printf("brk addr: %lx, a0: %lx\n", addr, trapframe->a0);
} 

void syscallSetBrk() {
    Trapframe *trapframe = getHartTrapFrame();
    u32 len = trapframe->a0;
    trapframe->a0 = sys_sbrk(len);
}

void syscallMapMemory() {
    Trapframe* trapframe = getHartTrapFrame();
    u64 start = trapframe->a0, len = trapframe->a1, prot = trapframe->a2,
        off = trapframe->a5, flags = trapframe->a3;
    struct File* fd;
    u64 perm = 0;
    if (prot & PROT_EXEC) {
        perm |= PTE_EXECUTE | PTE_READ;
    }
    if (prot & PROT_READ) {
        perm |= PTE_READ;
    }
    if (prot & PROT_WRITE) {
        perm |= PTE_WRITE | PTE_READ;
    }

    if (!len) {
        trapframe->a0 = -EINVAL;
        return;
    }
    argfd(4, 0, &fd);
    // printf("mmap: %lx %lx %lx %lx %d\n", start, len, prot, flags, fd);
    // printf("heap bottom: %lx\n", myProcess()->heapBottom);

    trapframe->a0 =
        do_mmap(fd, start, len, perm, /*'type' currently not used */ flags, off);
    // printf("mmap return value: %d\n", trapframe->a0);
    return;
}

void syscallUnMapMemory() {
    Trapframe *trapframe = getHartTrapFrame();
    u64 start = trapframe->a0, len = trapframe->a1, end = start + len;
    // printf("unmap: %lx %lx\n", start, len);
    start = UP_ALIGN(start, PAGE_SIZE);
    end = DOWN_ALIGN(end, PAGE_SIZE);
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
    strncpy(uname.release, "10.2.0", 65);
    strncpy(uname.version, "MIPS-OS", 65);
    strncpy(uname.machine, "Risc-V sifive_u", 65);
    strncpy(uname.domainname, "Beijing", 65);
    Trapframe *tf = getHartTrapFrame();
    copyout(myProcess()->pgdir, tf->a0, (char*)&uname, sizeof(struct utsname));
    tf->a0 = 0;
}

void syscallSetTidAddress() {
    Trapframe *tf = getHartTrapFrame();
    // copyout(myProcess()->pgdir, tf->a0, (char*)(&myProcess()->id), sizeof(u64));
    // printf("settid: %lx\n", tf->a0);
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
    copyin(p->pgdir, (char*)&set, tf->a1, tf->a3);
    if (tf->a2 != 0) {
        copyout(p->pgdir, tf->a2, (char*)(&myThread()->blocked), tf->a3);
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
    copyin(p->pgdir, (char*) &signalSet, tf->a0, tf->a3);
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
    u64 pid = tf->a0, resouce = tf->a1, newVa = tf->a2, oldVa = tf->a3;
    if (pid) {
        panic("Resource limit not current process!\n");
    }
    // printf("resource limit: pid: %lx, resource: %d, new: %lx, old: %lx\n", pid, resouce, newVa, oldVa);
    struct ResourceLimit newLimit;
    Process* process = myProcess();
    // acquireLock(&process->lock);
    if (newVa && copyin(process->pgdir, (char*)&newLimit, newVa, sizeof(struct ResourceLimit)) < 0) {
        // releaseLock(&process->lock);
        tf->a0 = -1;
    }
    switch(resouce) {
        case RLIMIT_NOFILE:
            if (newVa) {
                process->fileDescription.hard = newLimit.hard;
                process->fileDescription.soft = newLimit.soft;
            }
            if (oldVa) {
                copyout(process->pgdir, oldVa, (char*)&process->fileDescription, sizeof(struct ResourceLimit));
            }
            break;
    }
    // releaseLock(&process->lock);
    tf->a0 = 0;
}

void syscallIOControl() {
    Trapframe *tf = getHartTrapFrame();
    // printf("fd: %d, cmd: %d, argc: %d\n", tf->a0, tf->a1, tf->a2);
    tf->a0 = 0;
}

void syscallSocket() {
    Trapframe *tf = getHartTrapFrame();
    int domain = tf->a0, type = tf->a1, protocal = tf->a2;
    tf->a0 = createSocket(domain, type, protocal);
    // printf("[%s] socket at fd = %d\n",__func__, tf->a0);
}

void syscallBind() {
    Trapframe *tf = getHartTrapFrame();
    // assert(tf->a2 == sizeof(SocketAddr));
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
    tf->a0 = sendTo(myProcess()->ofile[tf->a0]->socket, buf, tf->a2, tf->a3, &sa);
}

void syscallReceiveFrom() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = receiveFrom(myProcess()->ofile[tf->a0]->socket, tf->a1, tf->a2, tf->a3, tf->a4);
}


void syscallListen() {
    Trapframe *tf = getHartTrapFrame();
    int sockfd = tf->a0;
    // printf("[%s] fd %d\n",__func__, sockfd);
    tf->a0 = listen(sockfd);
}

void syscallConnect() {
    Trapframe* tf = getHartTrapFrame();
    int sockfd = tf->a0;
    SocketAddr sa;
    copyin(myProcess()->pgdir, (char*)&sa, tf->a1, tf->a2);
    tf->a0 = connect(sockfd, &sa);
}

void syscallAccept() {
    Trapframe* tf = getHartTrapFrame();
    int sockfd = tf->a0;
    SocketAddr sa;
    tf->a0 = accept(sockfd, &sa);
    copyout(myProcess()->pgdir, tf->a1, (char*)&sa, sizeof(sa));
}

void syscallFutex() {
    Trapframe *tf = getHartTrapFrame();
    int op = tf->a1, val = tf->a2, userVal;
    u64 time = tf->a3;
    u64 uaddr = tf->a0, newAddr = tf->a4;
    struct TimeSpec t;
    // printf("addr: %lx, op: %d, val: %d, newAddr: %lx\n", uaddr, op, val, newAddr);
    op &= (FUTEX_PRIVATE_FLAG - 1);
    switch (op)
    {
        case FUTEX_WAIT:
            copyin(myProcess()->pgdir, (char*)&userVal, uaddr, sizeof(int));
            if (time) {
                if (copyin(myProcess()->pgdir, (char*)&t, time, sizeof(struct TimeSpec)) < 0) {
                    panic("copy time error!\n");
                }
            }
            // printf("val: %d\n", userVal);
            if (userVal != val) {
                tf->a0 = -1;
                return;
            }
            futexWait(uaddr, myThread(), time ? &t : 0);
            break;
        case FUTEX_WAKE:
            // printf("val: %d\n", val);
            futexWake(uaddr, val);
            break;
        case FUTEX_REQUEUE:
            // printf("val: %d\n", val);
            futexRequeue(uaddr, val, newAddr);
            break;
        default:
            panic("Futex type not support!\n");
    }
    tf->a0 = 0;
}

void syscallProcessKill() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = processSignalSend(tf->a0, tf->a1);
}

void syscallThreadKill() {
    Trapframe *tf = getHartTrapFrame();
    int tid = tf->a0, signal = tf->a1;
    tf->a0 = signalSend(0, tid, signal);
}

void syscallThreadGroupKill() {
    Trapframe *tf = getHartTrapFrame();
    int tgid = tf->a0, tid = tf->a1, signal = tf->a2;
    tf->a0 = signalSend(tgid, tid, signal);
}

void syscallPoll() {
    Trapframe *tf = getHartTrapFrame();
    struct pollfd {
        int fd;
        short events;
        short revents;
    };
    struct pollfd p;
    u64 startva = tf->a0;
    int n = tf->a1;
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        copyin(myProcess()->pgdir, (char*)&p, startva, sizeof(struct pollfd));
        p.revents = 0;
        copyout(myProcess()->pgdir, startva, (char*)&p, sizeof(struct pollfd));
        startva += sizeof(struct pollfd);
        cnt += p.revents != 0;
    }
    // tf->a0 = cnt;
    tf->a0 = 1;
}

void syscallMemoryProtect() {
    Trapframe *tf = getHartTrapFrame();
    // printf("mprotect va: %lx, length: %lx prot:%lx\n", tf->a0, tf->a1, tf->a2);

    u64 start = DOWN_ALIGN(tf->a0, PGSIZE);
    u64 end = UP_ALIGN(tf->a0+tf->a1, PGSIZE);

    u64 perm = 0;
    if (tf->a2 & PROT_EXEC) {
        perm |= PTE_EXECUTE | PTE_READ;
    }
    if (tf->a2 & PROT_READ) {
        perm |= PTE_READ;
    }
    if (tf->a2 & PROT_WRITE) {
        perm |= PTE_WRITE | PTE_READ;
    }
    while(start < end){
        u64 *pte, pa;
        pa = pageLookup(myProcess()->pgdir, start, &pte);
        if(!pa){
            pageout(myProcess()->pgdir, start);
        }else{
            *pte = (*pte & ~(PTE_READ | PTE_WRITE | PTE_EXECUTE)) | perm;
        }
        start += PGSIZE;
    }
    tf->a0 = 0;
}

void syscallGetRobustList() {
    Trapframe *tf = getHartTrapFrame();
    copyout(myProcess()->pgdir, tf->a1, (char*)&myThread()->robustHeadPointer, sizeof(u64));
    tf->a0 = 0;
}

void syscallSetRobustList() {
    Trapframe *tf = getHartTrapFrame();
    copyin(myProcess()->pgdir, (char*)&myThread()->robustHeadPointer, tf->a1, sizeof(u64));
    tf->a0 = 0;
}

void syscallStateFileSystem() {
    Trapframe *tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    if (argstr(0, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }
    FileSystemStatus fss;
    memset(&fss, 0, sizeof(FileSystemStatus));
    tf->a0 = getFsStatus(path, &fss);
    if (tf->a0 == 0) {
        // printf("bjoweihgre8i %ld\n", tf->a1);
        copyout(myProcess()->pgdir, tf->a1, (char*)&fss, sizeof(FileSystemStatus));
    }
}

void syscallGetUserId() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetEffectiveUserId() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;    
}

void syscallMemoryBarrier() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallSignalReturn() {
    Trapframe *tf = getHartTrapFrame();
    Thread* thread = myThread();
    SignalContext* sc = getHandlingSignal(thread);
    sc->contextRecover.epc = sc->uContext->uc_mcontext.MC_PC;
    thread->blocked = sc->uContext->uc_sigmask;
    bcopy(&sc->contextRecover, tf, sizeof(Trapframe));
    signalProcessEnd(sc->signal, &thread->processing);
    signalFinish(thread, sc);
}

void syscallLog() {
    Trapframe *tf = getHartTrapFrame();
    int type = tf->a0;
    u64 buf = tf->a1;
    u32 len = tf->a2;
    char tmp[] = "We havn't support syslog yet!\n";
    switch (type) {
    case SYSLOG_ACTION_READ_ALL:
        copyout(myProcess()->pgdir, buf, (char*)&tmp, MIN(sizeof(tmp), len));
        tf->a0 = MIN(sizeof(tmp), len);
        return;
    case SYSLOG_ACTION_SIZE_BUFFER:
        tf->a0 = sizeof(tmp);
        return;
    default:
        panic("%d\n", type);
        break;
    }
}

void syscallSystemInfo() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetResouceUsage() {
    Trapframe *tf = getHartTrapFrame();
    int who = tf->a0;
    u64 usage = tf->a1;
    assert(who == 0);

    struct rusage {
        struct TimeSpec ru_utime; /* user CPU time used */
        struct TimeSpec ru_stime; /* system CPU time used */
        long   ru_maxrss;        /* maximum resident set size */
        long   ru_ixrss;         /* integral shared memory size */
        long   ru_idrss;         /* integral unshared data size */
        long   ru_isrss;         /* integral unshared stack size */
        long   ru_minflt;        /* page reclaims (soft page faults) */
        long   ru_majflt;        /* page faults (hard page faults) */
        long   ru_nswap;         /* swaps */
        long   ru_inblock;       /* block input operations */
        long   ru_oublock;       /* block output operations */
        long   ru_msgsnd;        /* IPC messages sent */
        long   ru_msgrcv;        /* IPC messages received */
        long   ru_nsignals;      /* signals received */
        long   ru_nvcsw;         /* voluntary context switches */
        long   ru_nivcsw;        /* involuntary context switches */
    } rusage;

    rusage.ru_utime.second = myProcess()->cpuTime.user / 1000000;
    rusage.ru_utime.nanoSecond = myProcess()->cpuTime.user % 1000000 * 1000;
    rusage.ru_stime.second = myProcess()->cpuTime.kernel / 1000000;
    rusage.ru_stime.nanoSecond = myProcess()->cpuTime.kernel % 1000000 * 1000;

    // printf("usage: u: %ld.%ld, s: %ld.%ld\n", rusage.ru_utime.second, rusage.ru_utime.microSecond, rusage.ru_stime.second, rusage.ru_stime.microSecond);
    copyout(myProcess()->pgdir, usage, (char*)&rusage, sizeof (struct rusage));
    tf->a0 = 0;
}

void syscallSelect() {
    Trapframe *tf = getHartTrapFrame();
    // printf("thread %lx get in select, epc: %lx\n", myThread()->id, tf->epc);
    int nfd = tf->a0;
    // assert(nfd <= 128);
    u64 read = tf->a1, write = tf->a2, except = tf->a3/*, timeout = tf->a4*/;
    // assert(timeout != 0);
    int cnt = 0;
    struct File* file = NULL;
    // printf("[%s] \n", __func__);

    if (read) {
        FdSet readSet;
        copyin(myProcess()->pgdir, (char*)&readSet, read, sizeof(FdSet));
        for (int i = 0; i < nfd; i++) {
            file = NULL;
            u64 cur = i < 64 ? readSet.bits[0] & (1UL << i)
                             : readSet.bits[1] & (1UL << (i - 64));
            if (!cur)
                continue;
            // printf("selecting read fd %d type %d\n", i, myProcess()->ofile[i]->type);
            file = myProcess()->ofile[i];
            if (!file)
                continue;

            int ready_to_read = 1;
            switch (file->type) {
                case FD_PIPE:
                    // printf("[select] pipe:%lx nread: %d nwrite: %d\n", file->pipe, file->pipe->nread, file->pipe->nwrite);
                    if (file->pipe->nread == file->pipe->nwrite) {
                        ready_to_read = 0;
                    } else {
                        ready_to_read = 1;
                    }
                    break;
                case FD_SOCKET:
                    // printf("socketid %d head %d tail %d\n", file->socket-sockets, file->socket->head, file->socket->tail);
                    if (file->socket->used != 0 &&
                        file->socket->head == file->socket->tail &&
                        (!file->socket->listening ||
                         (file->socket->listening &&
                          file->socket->pending_h ==
                              file->socket->pending_t))) {
                        ready_to_read = 0;
                    } else {
                        ready_to_read = 1;
                    }
                    break;
                default:
                    ready_to_read = 1;
                    break;
            }

            if (ready_to_read) {
                ++cnt;
            } else {
                if (i < 64)
                    readSet.bits[0] &= ~cur;
                else
                    readSet.bits[1] &= ~cur;
            }
        }
        copyout(myProcess()->pgdir, read, (char*)&readSet, sizeof(FdSet));
    }
    if (write) {
        FdSet writeSet;
        copyin(myProcess()->pgdir, (char*)&writeSet, write, sizeof(FdSet));
        for (int i = 0; i < nfd; i++) {
            if (i >= 64) {
                cnt += !!((1UL << (i - 64)) & writeSet.bits[1]);
            } else {
                cnt += !!((1UL << (i)) & writeSet.bits[0]);
            }
        }
        copyout(myProcess()->pgdir, write, (char*)&write,
                sizeof(FdSet));
        
    }
    if (except) {
        FdSet set;
        // copyin(myProcess()->pgdir, (char*)&set, except, sizeof(FdSet));
        memset(&set, 0, sizeof(FdSet));
        copyout(myProcess()->pgdir, except, (char*)&set, sizeof(FdSet));
    }
    if (cnt == 0) {
        if (!(myThread()->reason & SELECT_BLOCK)) {
            myThread()->reason |= SELECT_BLOCK;
            tf->epc -= 4;
            yield();
        }
        myThread()->reason &= ~SELECT_BLOCK;
        
    }

    // printf("select end cnt %d\n",cnt);
    tf->a0 = cnt;
}

void syscallSetTimer() {
    Trapframe *tf = getHartTrapFrame();
    // printf("set Timer: %lx %lx %lx\n", tf->a0, tf->a1, tf->a2);
    IntervalTimer time = getTimer();
    if (tf->a2) {
        copyout(myProcess()->pgdir, tf->a2, (char*)&time, sizeof(IntervalTimer));
    }
    if (tf->a1) {
        copyin(myProcess()->pgdir, (char*)&time, tf->a1, sizeof(IntervalTimer));
        setTimer(time);
    }
    tf->a0 = 0;    
}

void syscallMemorySychronize() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallMemeryAdvise(){
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetGroupId() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetEffectiveGroupId() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = 0;
}

void syscallGetRandom() {
    Trapframe *tf = getHartTrapFrame();
    tf->a0 = r_time();
}
