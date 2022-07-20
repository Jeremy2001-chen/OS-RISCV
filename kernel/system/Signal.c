#include <Signal.h>
#include <Riscv.h>
#include <Process.h>
#include <Page.h>

int signProccessMask(u64 how, SignalSet *newSet) {
    Process *p = myproc();
    switch (how) {
    case SIG_UNBLOCK:
        p->blocked |= ~(*newSet);
        return 0;
    case SIG_BLOCK:
        p->blocked &= *newSet;
        return 0;
    case SIG_SETMASK:
        p->blocked = *newSet;
        return 0;
    default:
        return -1;
    }
}

int doSignalAction(int sig, u64 act, u64 oldAction) {
    Process *p = myproc();
    if (sig < 1 || sig > SIGNAL_COUNT) {
        return -1;
    }
	SignalAction *k = getSignalHandler(p) + (sig - 1);
    if (oldAction) {
        copyout(myproc()->pgdir, oldAction, (char*)k, sizeof(SignalAction));
    }
	if (act) {
		copyin(myproc()->pgdir, (char *)k, act, sizeof(SignalAction));
	}
	return 0;
}

int __dequeueSignal(SignalSet *pending, SignalSet *mask) {
    u64 x = *pending & ~*mask;
    if (x) {
        int i = 1;
        while (!(x & 1)) {
            i++;
            x >>= 1;
        }
        return i;
    }
    return 0;
}

static int dequeueSignal(Process *p, SignalSet *mask, SignalInfo *info) {
    int signal = __dequeueSignal(&p->pending, mask);
    return signal;
}

int doSignalTimedWait(SignalSet *which, SignalInfo *info, TimeSpec *ts) {
    Process *p = myproc();
    if (ts) {
        p->awakeTime = r_time() +  ts->second * 1000000 + ts->microSecond;
    }
    return dequeueSignal(p, which, info);    
}

void handleSignal(struct Process* process) {
    while(true) {
        int signal = dequeueSignal(process, &(process->blocked), NULL);
        if (!signal) {
            break;
        }
        switch (signal) {
            case SIGQUIT:
            case SIGKILL:
                processDestory(process);
            default:
                panic("Can not handle this %d of signal\n", signal);
                break;
        }
        process->pending -= (1 << signal);
    }
}