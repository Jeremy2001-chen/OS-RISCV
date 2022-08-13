#include <Signal.h>
#include <Riscv.h>
#include <Process.h>
#include <Page.h>
#include <Thread.h>
#include <Error.h>

SignalContext freeSignalContext[SIGNAL_CONTEXT_COUNT]; 
struct SignalContextList freeSignalContextList;
struct Spinlock signalContextListLock;

void signalInit() {
    initLock(&signalContextListLock, "signalContextLock");
    for (int i = 0; i < SIGNAL_CONTEXT_COUNT; i++) {
        LIST_INSERT_HEAD(&freeSignalContextList, &freeSignalContext[i], link);
    } 
}

void signalContextFree(SignalContext* sc) {
    // acquireLock(&signalContextListLock);
    LIST_INSERT_HEAD(&freeSignalContextList, sc, link);
    // releaseLock(&signalContextListLock);
}

int signalContextAlloc(SignalContext **signalContext) {
    // acquireLock(&signalContextListLock);
    SignalContext *sc;
    if ((sc = LIST_FIRST(&freeSignalContextList)) != NULL) {
        *signalContext = sc;
        LIST_REMOVE(sc, link);
        sc->start = false;
        // releaseLock(&signalContextListLock);
        return 0;
    }
    // releaseLock(&signalContextListLock);
    printf("there's no signal context left!\n");
    *signalContext = NULL;
    return -1;
}

int signalSend(int tgid, int tid, int sig) {
/* if tgid != 0, we will check if tid is belong to tgid. tgid is the same as processId */
    if (tid == -1) {
        panic("thread to group not support!\n");
    }
    Thread* thread;
    int r = tid2Thread(tid, &thread, 0);
    if (r < 0) {
        panic("");
        return -EINVAL;
    }
    if (tgid != 0 && thread->process->processId != tgid) {
        // 线程所属的线程组（进程）与tgid不相等
        return -ESRCH; // tid所代表的线程已经结束了
    }
    // if (!LIST_EMPTY(&thread->waitingSignal)) {
    //     // dangerous
    //     return 0;
    // }
    SignalContext* sc;
    r = signalContextAlloc(&sc);
    if (r < 0) {
        panic("");
    }
    // acquireLock(&thread->lock);
    sc->signal = sig;
    if (sig == SIGKILL) {
        thread->state = RUNNABLE;
    }
    LIST_INSERT_HEAD(&thread->waitingSignal, sc, link);
    // releaseLock(&thread->lock);
    return 0;
}

int processSignalSend(int pid, int sig) {
    extern Thread threads[];
    int ret = -ESRCH;
    for (int i = 0; i < PROCESS_TOTAL_NUMBER; i++) {
        // acquireLock(&threads[i].lock);
        if (threads[i].state != UNUSED) {
            if (pid == 0 || pid == -1 || pid == threads[i].process->processId) {
                // releaseLock(&threads[i].lock);
                ret = signalSend(0, threads[i].id, sig);
                ret = ret == 0 ? 0 : ret;
                continue;
            }
        }
        // releaseLock(&threads[i].lock);
    }
    return ret;
}

int signProccessMask(u64 how, SignalSet *newSet) {
    Thread* th = myThread();
    switch (how) {
    case SIG_BLOCK:
        th->blocked.signal[0] |= newSet->signal[0];
        return 0;
    case SIG_UNBLOCK:
        th->blocked.signal[0] &= ~(newSet->signal[0]);
        return 0;
    case SIG_SETMASK:
        th->blocked.signal[0] = newSet->signal[0];
        return 0;
    default:
        return -1;
    }
}

int doSignalAction(int sig, u64 act, u64 oldAction) {
    Thread* th = myThread();
    if (sig < 1 || sig > SIGNAL_COUNT) {
        return -1;
    }
	SignalAction *k = getSignalHandler(th->process) + (sig - 1);
    if (oldAction) {
        copyout(myProcess()->pgdir, oldAction, (char*)k, sizeof(SignalAction));
    }
	if (act) {
		copyin(myProcess()->pgdir, (char *)k, act, sizeof(SignalAction));
	}
	return 0;
}

SignalContext* getFirstSignalContext(Thread* thread) {
    SignalContext* sc = NULL;
    // acquireLock(&thread->lock);
    LIST_FOREACH(sc, &thread->waitingSignal, link) {
        if (!sc->start && (signalSetAnd(sc->signal, &thread->blocked) || signalSetAnd(sc->signal, &thread->processing))) {
            continue;
        }
        break;
    }
    // releaseLock(&thread->lock);
    return sc;
}

SignalContext* getHandlingSignal(Thread* thread) {
    SignalContext* sc = NULL;
    // acquireLock(&thread->lock);
    LIST_FOREACH(sc, &thread->waitingSignal, link) {
        if (sc->start) {
            break;
        }
    }
    // releaseLock(&thread->lock);
    assert(sc != NULL);
    return sc;
}

bool hasKillSignal(Thread* thread) {
    SignalContext* sc = NULL;
    // acquireLock(&thread->lock);
    bool find = false;
    LIST_FOREACH(sc, &thread->waitingSignal, link) {
        if (sc->signal == SIGKILL) {
            find = true;
            break;
        }
    }
    // releaseLock(&thread->lock);
    return find;
}


void initFrame(SignalContext* sc, Thread* thread) {
    Trapframe* tf = getHartTrapFrame();
    u64 sp = DOWN_ALIGN(tf->sp - PAGE_SIZE, PAGE_SIZE);
    PhysicalPage *page;
    int r;
    if ((r = pageAlloc(&page)) < 0) {
        panic("");
    }
    pageInsert(myProcess()->pgdir, sp - PAGE_SIZE, page2pa(page), PTE_USER | PTE_READ | PTE_WRITE);
    u32 pageTop = PAGE_SIZE;
    pageTop -= sizeof(SignalInfo);
    u64 sigInfo;
    sigInfo = pageTop = DOWN_ALIGN(pageTop, 16);
    SignalInfo si = {0};
    bcopy(&si, (void*)page2pa(page) + pageTop, sizeof(SignalInfo));
    pageTop -= sizeof(ucontext);
    u64 uContext;
    uContext = pageTop = DOWN_ALIGN(pageTop, 16);
    sc->uContext = (ucontext*) (uContext + page2pa(page));
    ucontext uc = {0};
    uc.uc_sigmask = thread->blocked;
    uc.uc_mcontext.MC_PC = tf->epc;
    bcopy(&uc, (void*)page2pa(page) + pageTop, sizeof(ucontext));
    tf->a0 = sc->signal;
    tf->a1 = sigInfo + sp - PAGE_SIZE;
    tf->a2 = uContext + sp - PAGE_SIZE;
    tf->sp = pageTop + sp - PAGE_SIZE;
    tf->ra = SIGNAL_TRAMPOLINE_BASE;
}

void signalFinish(Thread* thread, SignalContext* sc) {
    // acquireLock(&thread->lock);
    LIST_REMOVE(sc, link);
    // releaseLock(&thread->lock);
    signalContextFree(sc);
}

void handleSignal(Thread* thread) {
    SignalContext* sc;
    while (1) {
        sc = getFirstSignalContext(thread);
        if (sc == NULL) {
            return;
        }
        if (hasKillSignal(thread)) {
            threadDestroy(thread);
            return;
        }
        if (sc->start) {
            return;
        }
        SignalAction *sa = getSignalHandler(thread->process) + (sc->signal - 1);
        if (sa->handler == NULL) {
            signalFinish(thread, sc);
            continue;
        }
        sc->start = true;
        signalProcessStart(sc->signal, &thread->processing);
        Trapframe* tf = getHartTrapFrame();
        bcopy(tf, &sc->contextRecover, sizeof(Trapframe));
        struct pthread self;
        copyin(thread->process->pgdir, (char*)&self, thread->trapframe.tp - sizeof(struct pthread), sizeof(struct pthread));
        initFrame(sc, thread);
        tf->epc = (u64)sa->handler;
        return;
    }
}

int doSignalTimedWait(SignalSet *which, SignalInfo *info, TimeSpec *ts) {
    Thread* thread = myThread();
    // if (ts) {
    //     thread->awakeTime = r_time() +  ts->second * 1000000 + ts->nanoSecond / 1000;
    // }
    SignalContext *sc = getFirstSignalContext(thread);
    return sc == NULL ? 0 : sc->signal;    
}
