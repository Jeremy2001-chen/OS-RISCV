// Sleeping locks

#include "Sleeplock.h"
#include "Process.h"
#include "Spinlock.h"

#define SINGLE_PROCESS

#ifdef SINGLE_PROCESS //
void initsleeplock(struct Sleeplock* lk, char* name) {}
void acquiresleep(struct Sleeplock* lk) {}
void releasesleep(struct Sleeplock* lk) {}
int holdingsleep(struct Sleeplock* lk) {
    return 1;
}
#else

void initsleeplock(struct Sleeplock* lk, char* name) {
    initLock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->pid = 0;
}

void acquiresleep(struct Sleeplock* lk) {
    acquireLock(&lk->lk);
    while (lk->locked) {
        sleep(lk, &lk->lk);
    }
    lk->locked = 1;
    lk->pid = myproc()->id;
    releaseLock(&lk->lk);
}

void releasesleep(struct Sleeplock* lk) {
    acquireLock(&lk->lk);
    lk->locked = 0;
    lk->pid = 0;
    wakeup(lk);
    releaseLock(&lk->lk);
}

int holdingsleep(struct Sleeplock* lk) {
    int r;

    acquireLock(&lk->lk);
    r = lk->locked && (lk->pid == myproc()->id);
    releaseLock(&lk->lk);
    return r;
}
#endif