#include <Signal.h>
#include <Riscv.h>
#include <Process.h>
#include <Page.h>
#include <Thread.h>

int signProccessMask(u64 how, SignalSet *newSet) {
    Thread* th = myThread();
    switch (how) {
    case SIG_BLOCK:
        th->blocked |= *newSet;
        return 0;
    case SIG_UNBLOCK:
        th->blocked &= ~(*newSet);
        return 0;
    case SIG_SETMASK:
        th->blocked = *newSet;
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
	SignalAction *k = getSignalHandler(th) + (sig - 1);
    if (oldAction) {
        copyout(myProcess()->pgdir, oldAction, (char*)k, sizeof(SignalAction));
    }
	if (act) {
		copyin(myProcess()->pgdir, (char *)k, act, sizeof(SignalAction));
	}
	return 0;
}

int __dequeueSignal(SignalSet *pending, SignalSet *mask) {
    u64 x = *pending & ~*mask;
    if (x) {
        int i = 0;
        while (!(x & 1)) {
            i++;
            x >>= 1;
        }
        return i;
    }
    return 0;
}

static int dequeueSignal(Thread *th, SignalSet *mask, SignalInfo *info) {
    int signal = __dequeueSignal(&th->pending, mask);
    return signal;
}

int doSignalTimedWait(SignalSet *which, SignalInfo *info, TimeSpec *ts) {
    Thread* thread = myThread();
    if (ts) {
        thread->awakeTime = r_time() +  ts->second * 1000000 + ts->microSecond;
    }
    return dequeueSignal(thread, which, info);    
}

void signalCancel(Thread* thread) {
    struct pthread self;
    copyin(thread->process->pgdir, (char*)&self, thread->trapframe.tp - sizeof(struct pthread), sizeof(struct pthread));
    if (self.cancelasync && !self.canceldisable) {
        //pthread_exit
        panic("%d\n", self.tsd_used);
    }
}

void handleSignal(Thread* thread) {
    for (int i = 0; i < 64; i++) {
        u64 signal = (thread->pending & ~thread->blocked);
        if ((1ul << i) & signal) {
            switch (i) {
                case SIGQUIT:
                case SIGKILL:
                    threadDestroy(thread);
                    thread->pending -= (1ul << i);
                    break;
                case SIGCANCEL:
                    signalCancel(thread);
                    break;
                default:
                    panic("Can not handle this %d of signal\n", i);
                    break;
                }
        }
    }
}