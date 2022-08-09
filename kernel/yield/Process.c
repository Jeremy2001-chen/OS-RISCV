#include <Process.h>
#include <bio.h>
#include <Page.h>
#include <Error.h>
#include <Elf.h>
#include <Riscv.h>
#include <Trap.h>
#include <string.h>
#include <Spinlock.h>
#include <Interrupt.h>
#include <Debug.h>
#include <FileSystem.h>
#include <Sysfile.h>
#include <Signal.h>
#include <Thread.h>
#include <Sysfile.h>
#include <Wait.h>

Process processes[PROCESS_TOTAL_NUMBER];
static struct ProcessList freeProcesses;

struct Spinlock freeProcessesLock, processIdLock, waitLock, currentProcessLock;

Process* myProcess() {
    // interruptPush();
    if (myThread() == NULL) {
        // interruptPop();
        return NULL;
    }
    // interruptPop();
    return myThread()->process;
}

SignalAction *getSignalHandler(Process* p) {
    return (SignalAction*)(PROCESS_SIGNAL_BASE + (u64)(p - processes) * PAGE_SIZE);
}

extern Trapframe trapframe[];

void processInit() {
    printf("Process init start...\n");
    
    initLock(&freeProcessesLock, "freeProcess");
    initLock(&processIdLock, "processId");
    initLock(&waitLock, "waitProcess");

    LIST_INIT(&freeProcesses);
    
    int i;
    // extern u64 kernelPageDirectory[];
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);
    }

    threadInit();
    w_sscratch((u64)getHartTrapFrame());
    printf("Process init finish!\n");
}

u32 generateProcessId(Process *p) {
    acquireLock(&processIdLock);
    static u32 nextId = 0;
    u32 processId = (++nextId << (1 + LOG_PROCESS_NUM)) | (u32)(p - processes);
    releaseLock(&processIdLock);
    return processId;
}

void processFree(Process *p) {
    // printf("[%lx] free env %lx\n", currentProcess[r_hartid()] ? currentProcess[r_hartid()]->id : 0, p->id);
    pgdirFree(p->pgdir);
    p->state = ZOMBIE; // new
    for (int fd = 0; fd < NOFILE; fd++) {
        if (p->ofile[fd]) {
            struct File* f = p->ofile[fd];
            fileclose(f);
            p->ofile[fd] = 0;
        }
    }
    kernelProcessCpuTimeEnd();
    if (p->parentId > 0) {
        Process* parentProcess;
        int r = pid2Process(p->parentId, &parentProcess, 0);
        // if (r < 0) {
        //     panic("Can't get parent process, current process is %x, parent is %x\n", p->id, p->parentId);
        // }
        // printf("[Free] process %x wake up %x\n", p->id, parentProcess);
        // The parent process may die before the child process
        if (r == 0) {
            wakeup(parentProcess);
        }
    }
}

int pid2Process(u32 processId, struct Process **process, int checkPerm) {
    struct Process* p;
    // int hartId = r_hartid();

    if (processId == 0) {
        *process = myProcess();
        return 0;
    }

    p = processes + PROCESS_OFFSET(processId);

    if (p->state == UNUSED || p->processId != processId) {
        *process = NULL;
        return -INVALID_PROCESS_STATUS;
    }

    if (checkPerm) {
        if (p != myProcess() && p->parentId != myProcess()->processId) {
            *process = NULL;
            return -INVALID_PERM;
        }
    }

    *process = p;
    return 0;
}

extern FileSystem rootFileSystem;
extern void userVector();
int processSetup(Process *p) {
    int r;
    PhysicalPage *page;
    r = allocPgdir(&page);
    if (r < 0) {
        panic("setup page alloc error\n");
        return r;
    }
    
    p->pgdir = (u64*) page2pa(page);
    p->retValue = 0;
    p->state = UNUSED;
    p->parentId = 0;
    p->heapBottom = USER_HEAP_BOTTOM;
    p->cwd = &rootFileSystem.root;
    p->fileDescription.hard = p->fileDescription.soft = NOFILE;

    extern char trampoline[];
    pageInsert(p->pgdir, TRAMPOLINE_BASE, (u64)trampoline, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE);
    pageInsert(p->pgdir, TRAMPOLINE_BASE + PAGE_SIZE, ((u64)trampoline) + PAGE_SIZE, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE);
    extern char signalTrampoline[];
    pageInsert(p->pgdir, SIGNAL_TRAMPOLINE_BASE, (u64)signalTrampoline,
        PTE_USER | PTE_EXECUTE | PTE_READ | PTE_WRITE);

    if (pageAlloc(&page) < 0) {
        panic("");
    }
    extern u64 kernelPageDirectory[];
    pageInsert(kernelPageDirectory, (u64)getSignalHandler(p), page2pa(page), PTE_READ | PTE_WRITE);
    return 0;
}

int processAlloc(Process **new, u64 parentId) {
    int r;
    Process *p;
    acquireLock(&freeProcessesLock);
    if (LIST_EMPTY(&freeProcesses)) {
        releaseLock(&freeProcessesLock);
        *new = NULL;
        return -NO_FREE_PROCESS;
    }
    p = LIST_FIRST(&freeProcesses);
    LIST_REMOVE(p, link);
    // printf("[Process Alloc] alloc an process %d, next : %x\n", (u32)(p - processes), (u32)(LIST_FIRST(&freeProcesses) - processes));
    releaseLock(&freeProcessesLock);
    if ((r = processSetup(p)) < 0) {
        return r;
    }

    p->processId = generateProcessId(p);
    p->state = RUNNABLE;
    p->parentId = parentId;

    *new = p;
    return 0;
}

int codeMapper(u64 va, u32 segmentSize, u8 *binary, u32 binSize, void *userData) {
    Process *process = (Process*)userData;
    PhysicalPage *p = NULL;
    u64 i;
    int r = 0;
    u64 offset = va - DOWN_ALIGN(va, PAGE_SIZE);
    u64* j;

    if (offset > 0) {
        p = pa2page(pageLookup(process->pgdir, va, &j));
        if (p == NULL) {
            if (pageAlloc(&p) < 0) {
                return -1;
            }
            pageInsert(process->pgdir, va, page2pa(p), 
                PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(binSize, PAGE_SIZE - offset);
        bcopy(binary, (void*) page2pa(p) + offset, r);
    }
    for (i = r; i < binSize; i += r) {
        if (pageAlloc(&p) != 0) {
            return -1;
        }
        pageInsert(process->pgdir, va + i, page2pa(p), 
            PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, binSize - i);
        bcopy(binary + i, (void*) page2pa(p), r);
    }

    offset = va + i - DOWN_ALIGN(va + i, PAGE_SIZE);
    if (offset > 0) {
        p = pa2page(pageLookup(process->pgdir, va + i, &j));
        if (p == NULL) {
            if (pageAlloc(&p) != 0) {
                return -1;
            }
            pageInsert(process->pgdir, va + i, page2pa(p), 
                PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(segmentSize - i, PAGE_SIZE - offset);
        bzero((void*) page2pa(p) + offset, r);
    }
    for (i += r; i < segmentSize; i += r) {
        if (pageAlloc(&p) != 0) {
            return -1;
        }
        pageInsert(process->pgdir, va + i, page2pa(p), 
            PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, segmentSize - i);
        bzero((void*) page2pa(p), r);
    }
    return 0;
}

extern struct Spinlock scheduleListLock;
extern struct ThreadList scheduleList[2];
void processCreatePriority(u8 *binary, u32 size, u32 priority) {
    Thread* th;
    int r = mainThreadAlloc(&th, 0);
    if (r < 0) {
        return;
    }
    Process* p = th->process;
    p->priority = priority;
    u64 entryPoint;
    if (loadElf(binary, size, &entryPoint, p, codeMapper) < 0) {
        panic("process create error\n");
    }
    th->trapframe.epc = entryPoint;

    acquireLock(&scheduleListLock);
    LIST_INSERT_TAIL(&scheduleList[0], th, scheduleLink);
    releaseLock(&scheduleListLock);
}

static inline void updateAncestorsCpuTime(Process *p) {
    Process *pp = p;
    while (pp->parentId > 0 && pid2Process(pp->parentId, &pp, false) >= 0) {
        pp->cpuTime.deadChildrenKernel += p->cpuTime.kernel;
        pp->cpuTime.deadChildrenUser += p->cpuTime.user;
    }
}

int wait(int targetProcessId, u64 addr, int flags) {
    Process* p = myProcess();
    int haveChildProcess, pid;

    acquireLock(&waitLock);

    while (true) {
        haveChildProcess = 0;
        for (int i = 0; i < PROCESS_TOTAL_NUMBER; ++i) {
            Process* np = &processes[i];
            acquireLock(&np->lock);
            if (np->state != UNUSED && np->parentId == p->processId) {
                haveChildProcess = 1;
                if ((targetProcessId == -1 || np->processId == targetProcessId) && np->state == ZOMBIE) {
                    pid = np->processId;
                    if (addr != 0 && copyout(p->pgdir, addr, (char *)&np->retValue, sizeof(np->retValue)) < 0) {
                        releaseLock(&np->lock);
                        releaseLock(&waitLock);
                        return -1;
                    }
                    acquireLock(&freeProcessesLock);
                    updateAncestorsCpuTime(np);
                    np->state = UNUSED;
                    LIST_INSERT_HEAD(&freeProcesses, np, link); 
                    // printf("[Process Free] Free an process %d\n", (u32)(np - processes));
                    releaseLock(&freeProcessesLock);
                    releaseLock(&np->lock);
                    releaseLock(&waitLock);
                    return pid;
                }
            }
            releaseLock(&np->lock);
        }

        if (!haveChildProcess) {
            releaseLock(&waitLock);
            return -1;
        }

        if (flags == WNOHANG) {
            releaseLock(&waitLock);
            return 0;    
        }
        // printf("[WAIT]porcess id %x wait for %x\n", p->id, p);
        sleep(p, &waitLock);
    }
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, u64 dst, void* src, u64 len) {
    if (user_dst) {
        struct Process* p = myProcess();//because only this branch uses p->pgdir, so it need call myProcess
        return copyout(p->pgdir, dst, src, len);
    } else {
        memmove((char*)dst, src, len);
        return 0;
    }
}

int either_memset(bool user, u64 dst, u8 value, u64 len) {
    if (user) {
        Process *p = myProcess();
        return memsetOut(p->pgdir, dst, value, len);
    }
    memset((void *)dst, value, len);
    return 0;
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void* dst, int user_src, u64 src, u64 len) {
    if (user_src) {
        struct Process* p = myProcess();//because only this branch uses p->pgdir, so it need call myProcess
        return copyin(p->pgdir, dst, src, len);
    } else {
        memmove(dst, (char*)src, len);
        return 0;
    }
}

void kernelProcessCpuTimeBegin() {
    Process *p = myProcess();
    long currentTime = r_time();
    p->cpuTime.kernel += currentTime - p->processTime.lastKernelTime;
}

void kernelProcessCpuTimeEnd() {
    Process *p = myProcess();
    p->processTime.lastKernelTime = r_time();
}
