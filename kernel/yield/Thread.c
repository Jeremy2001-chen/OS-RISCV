#include <Thread.h>
#include <Riscv.h>
#include <MemoryConfig.h>
#include <Error.h>
#include <FileSystem.h>
#include <Sysfile.h>
#include <Page.h>
#include <Interrupt.h>
#include <Process.h>
#include <Trap.h>
#include <Futex.h>

Thread threads[PROCESS_TOTAL_NUMBER];

static struct ThreadList freeThreades;
struct ThreadList scheduleList[2];
Thread *currentThread[HART_TOTAL_NUMBER] = {0};

struct Spinlock freeThreadListLock, scheduleListLock, threadIdLock;

Thread* myThread() {
    interruptPush();
    int hartId = r_hartid();
    if (currentThread[hartId] == NULL) {
        // printf("[Kernel]No thread run in the hart %d\n", hartId);
        interruptPop();
        return NULL;
    }
    Thread* ret = currentThread[hartId];
    interruptPop();
    return ret;
}

u64 getThreadTopSp(Thread* th) {
    return KERNEL_PROCESS_SP_TOP - (u64)(th - threads) * 10 * PAGE_SIZE;
}

SignalAction *getSignalHandler(Thread* th) {
    return (SignalAction*)(PROCESS_SIGNAL_BASE + (u64)(th - threads) * PAGE_SIZE);
}

extern u64 kernelPageDirectory[];
void threadInit() {
    initLock(&freeThreadListLock, "freeThread");
    initLock(&scheduleListLock, "scheduleList");
    initLock(&threadIdLock, "threadId");

    LIST_INIT(&freeThreades);
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);

    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        threads[i].state = UNUSED;
        threads[i].trapframe.kernelSatp = MAKE_SATP(kernelPageDirectory);
        LIST_INSERT_HEAD(&freeThreades, &threads[i], link);
    }
}

u32 generateThreadId(Thread* th) {
    acquireLock(&threadIdLock);
    static u32 nextId = 0;
    u32 threadId = (++nextId << (1 + LOG_PROCESS_NUM)) | (u32)(th - threads);
    releaseLock(&threadIdLock);
    return threadId;
}

void sleepRec();
void threadDestroy(Thread *th) {
    threadFree(th);
    int hartId = r_hartid();
    if (currentThread[hartId] == th) {
        currentThread[hartId] = NULL;
        extern char kernelStack[];
        u64 sp = (u64)kernelStack + (hartId + 1) * KERNEL_STACK_SIZE;
        asm volatile("ld sp, 0(%0)" : :"r"(&sp): "memory");
        yield();
    }
}

void threadFree(Thread *th) {
    Process* p = th->process;
    acquireLock(&p->lock);
    if (th->clearChildTid) {
        int val = 0;
        copyout(p->pgdir, th->clearChildTid, (char*)&val, sizeof(int));
        futexWake(th->clearChildTid, 1);
    }
    p->threadCount--;
    if (!p->threadCount) {
        p->retValue = th->retValue;
        releaseLock(&p->lock);
        processFree(p);    
    } else {
        releaseLock(&p->lock);
    }

    acquireLock(&freeThreadListLock);
    th->state = UNUSED;
    LIST_INSERT_HEAD(&freeThreades, th, link); //test pipe
    releaseLock(&freeThreadListLock);
}

int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm) {
    struct Thread* th;
    int hartId = r_hartid();

    if (threadId == 0) {
        *thread = currentThread[hartId];
        return 0;
    }

    th = threads + PROCESS_OFFSET(threadId);

    if (th->state == UNUSED || th->id != threadId) {
        *thread = NULL;
        return -ESRCH;
    }

    if (checkPerm) {
        if (th != currentThread[hartId] && th->process->parentId != myProcess()->processId) {
            *thread = NULL;
            return -EPERM;
        }
    }

    *thread = th;
    return 0;
}

extern FileSystem rootFileSystem;
extern void userVector();

void threadSetup(Thread* th) {
    th->chan = 0;
    th->retValue = 0;
    th->state = UNUSED;
    th->reason = 0;
    th->setChildTid = th->clearChildTid = 0;
    th->awakeTime = 0;
    th->robustHeadPointer = 0;
    PhysicalPage *page;
    if (pageAlloc(&page) < 0) {
        panic("");
    }
    pageInsert(kernelPageDirectory, getThreadTopSp(th) - PAGE_SIZE, page2pa(page), PTE_READ | PTE_WRITE);
    if (pageAlloc(&page) < 0) {
        panic("");
    }
    pageInsert(kernelPageDirectory, (u64)getSignalHandler(th), page2pa(page), PTE_READ | PTE_WRITE);
}

int mainThreadAlloc(Thread **new, u64 parentId) {
    int r;
    Thread *th;
    acquireLock(&freeThreadListLock);
    if (LIST_EMPTY(&freeThreades)) {
        releaseLock(&freeThreadListLock);
        *new = NULL;
        return -ESRCH;
    }
    th = LIST_FIRST(&freeThreades);
    LIST_REMOVE(th, link);
    releaseLock(&freeThreadListLock);

    threadSetup(th);
    th->id = generateThreadId(th);
    th->state = RUNNABLE;
    th->trapframe.kernelSp = getThreadTopSp(th);
    th->trapframe.sp = USER_STACK_TOP - 24; //argc = 0, argv = 0, envp = 0
    Process *process;
    r = processAlloc(&process, parentId);
    if (r < 0) {
        *new = NULL;
        return r;
    }
    acquireLock(&process->lock);
    th->process = process;
    process->threadCount++;
    releaseLock(&process->lock);
    *new = th;
    return 0;
}

int threadAlloc(Thread **new, Process* process, u64 userSp) {
    Thread *th;
    acquireLock(&freeThreadListLock);
    if (LIST_EMPTY(&freeThreades)) {
        releaseLock(&freeThreadListLock);
        *new = NULL;
        return -ESRCH;
    }
    th = LIST_FIRST(&freeThreades);
    LIST_REMOVE(th, link);
    releaseLock(&freeThreadListLock);

    threadSetup(th);
    th->id = generateThreadId(th);
    th->state = RUNNABLE;
    th->trapframe.kernelSp = getThreadTopSp(th);
    th->trapframe.sp = userSp;

    acquireLock(&process->lock);
    th->process = process;
    process->threadCount++;
    releaseLock(&process->lock);

    *new = th;
    return 0;
}

void threadRun(Thread* th) {
    static volatile int first = 0;
    Trapframe* trapframe = getHartTrapFrame();
    if (currentThread[r_hartid()]) {
        bcopy(trapframe, &(currentThread[r_hartid()]->trapframe),
              sizeof(Trapframe));
    }
    
    th->state = RUNNING;
    if (th->reason == KERNEL_GIVE_UP) {
        th->reason = NORMAL;
        currentThread[r_hartid()] = th;
        bcopy(&currentThread[r_hartid()]->trapframe, trapframe, sizeof(Trapframe));
        asm volatile("ld sp, 0(%0)" : : "r"(&th->currentKernelSp));
        sleepRec();
    } else {
        currentThread[r_hartid()] = th;
        if (first == 0) {
            // File system initialization must be run in the context of a
            // regular process (e.g., because it calls sleep), and thus cannot
            // be run from main().
            first = 1;
            strncpy(rootFileSystem.name, "fat32", 6);
            
            rootFileSystem.read = blockRead;
            fatInit(&rootFileSystem);
            initDirentCache();
            void testfat();
            testfat();

            struct dirent* ep = create(AT_FDCWD, "/dev", T_DIR, O_RDONLY);
            eunlock(ep);
            eput(ep);
            ep = create(AT_FDCWD, "/dev/vda2", T_DIR, O_RDONLY); //driver
            ep->head = &rootFileSystem;            
            eunlock(ep);
            eput(ep);
            ep = create(AT_FDCWD, "/dev/shm", T_DIR, O_RDONLY); //share memory
            eunlock(ep);
            eput(ep);
            ep = create(AT_FDCWD, "/dev/null", T_CHAR, O_RDONLY); //share memory
            eunlock(ep);
            eput(ep);
            ep = create(AT_FDCWD, "/tmp", T_DIR, O_RDONLY); //share memory
            eunlock(ep);
            eput(ep);
            ep = create(AT_FDCWD, "/dev/zero", T_CHAR, O_RDONLY);
            ep->dev = ZERO;
            eunlock(ep);
            printf("begin pre_link\n");
            int ret;
            if((ret=do_linkat(AT_FDCWD, "/libdlopen_dso.so", AT_FDCWD, "/dlopen_dso.so"))<0){
                printf("pre_link error\n");
            }            
            setNextTimeout();
        }
        bcopy(&(currentThread[r_hartid()]->trapframe), trapframe, sizeof(Trapframe));
        u64 sp = getHartKernelTopSp(th);
        asm volatile("ld sp, 0(%0)" : :"r"(&sp): "memory");
        // releaseLock(&currentProcessLock);
        userTrapReturn();
    }
}

void sleepSave();
void sleep(void* chan, struct Spinlock* lk) { 
    Thread* th = myThread();

    // Must acquire p->lock in order to
    // change p->state and then call sched.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.

    kernelProcessCpuTimeEnd();
    acquireLock(&th->lock);  // DOC: sleeplock1
    releaseLock(lk);

    // Go to sleep.
    th->chan = (u64)chan;
    th->state = SLEEPING;
    th->reason = KERNEL_GIVE_UP;
    releaseLock(&th->lock);

	asm volatile("sd sp, 0(%0)" : :"r"(&th->currentKernelSp));

    sleepSave();

    // // Tidy up.
    acquireLock(&th->lock);  // DOC: sleeplock1
    th->chan = 0;
    releaseLock(&th->lock);

    kernelProcessCpuTimeBegin();

    // Reacquire original lock.
    acquireLock(lk);
}

void wakeup(void* channel) {
    for (int i = 0; i < PROCESS_TOTAL_NUMBER; ++i) {
        if (&threads[i] != myThread()) {
            acquireLock(&threads[i].lock);
            if (threads[i].state == SLEEPING && threads[i].chan == (u64)channel) {
                threads[i].state = RUNNABLE;
            }
            releaseLock(&threads[i].lock);
        }
    }
}

