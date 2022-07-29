
#include <Thread.h>
#include <Process.h>
#include <Page.h>

extern struct Spinlock scheduleListLock;
extern struct ThreadList scheduleList[2];
int processFork() {
    Thread* thread;
    Process* process, *current = myProcess();
    int r = mainThreadAlloc(&thread, current->processId);
    if (r < 0) {
        return r;
    }
    process = thread->process;
    process->cwd = current->cwd;
    for (int i = 0; i < NOFILE; i++)
        if (current->ofile[i])
            process->ofile[i] = filedup(current->ofile[i]);
    process->priority = current->priority;
    Trapframe* trapframe = getHartTrapFrame();
    bcopy(trapframe, &thread->trapframe, sizeof(Trapframe));
    thread->trapframe.a0 = 0;
    thread->trapframe.kernelSp = getThreadTopSp(thread);
    u64 i, j, k;
    for (i = 0; i < 512; i++) {
        if (!(current->pgdir[i] & PTE_VALID)) {
            continue;
        }
        u64 *pa = (u64*) PTE2PA(current->pgdir[i]);
        for (j = 0; j < 512; j++) {
            if (!(pa[j] & PTE_VALID)) {
                continue;
            }
            u64 *pa2 = (u64*) PTE2PA(pa[j]);
            for (k = 0; k < 512; k++) {
                if (!(pa2[k] & PTE_VALID)) {
                    continue;
                }
                u64 va = (i << 30) + (j << 21) + (k << 12);
                // printf("Fork va addr is %lx\n", va);
                if (va == TRAMPOLINE_BASE || va == TRAMPOLINE_BASE + PAGE_SIZE) {
                    continue;
                }
                if (pa2[k] & PTE_WRITE) {
                    pa2[k] |= PTE_COW;
                    pa2[k] &= ~PTE_WRITE;
                } 
                pageInsert(process->pgdir, va, PTE2PA(pa2[k]), PTE2PERM(pa2[k]));
            }
        }
    }
    acquireLock(&scheduleListLock);
    LIST_INSERT_TAIL(&scheduleList[0], thread, scheduleLink);
    releaseLock(&scheduleListLock);
    return process->processId;
}

int threadFork(u64 stackVa, u64 ptid, u64 tls, u64 ctid) {
    Thread* thread;
    Process* current = myProcess();
    int r = threadAlloc(&thread, current, stackVa);
    if (r < 0) {
        return r;
    }
    Trapframe* trapframe = getHartTrapFrame();
    bcopy(trapframe, &thread->trapframe, sizeof(Trapframe));
    thread->trapframe.a0 = 0;
    thread->trapframe.tp = tls;
    thread->trapframe.kernelSp = getThreadTopSp(thread);
    thread->trapframe.sp = stackVa;
    if (ptid != 0) {
        copyout(current->pgdir, ptid, (char*) &thread->id, sizeof(u32));
    }
    thread->clearChildTid = ctid;
    acquireLock(&scheduleListLock);
    LIST_INSERT_TAIL(&scheduleList[0], thread, scheduleLink);
    releaseLock(&scheduleListLock);
    return thread->id;
}

int clone(u32 flags, u64 stackVa, u64 ptid, u64 tls, u64 ctid) {
    // printf("clone flags: %d\n", flags);
    if (flags == PROCESS_FORK) {
        return processFork();
    } else {
        return threadFork(stackVa, ptid, tls, ctid);
    }
}
