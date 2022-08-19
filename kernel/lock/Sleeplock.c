// Sleeping locks

#include "Sleeplock.h"
#include "Process.h"
#include "Spinlock.h"
#include "Driver.h"
#include <Debug.h>
#include <Thread.h>

void initsleeplock(struct Sleeplock* lk, char* name) {
    initLock(&lk->lk, "sleep lock");
    lk->name = name;
    lk->locked = 0;
    lk->tid = 0;
}

void acquiresleep(struct Sleeplock* lk) {
    // acquireLock(&lk->lk);
    while (lk->locked) {
        // MSG_PRINT("in while");
        sleep(lk, &lk->lk);
    }
    lk->locked = 1;
    lk->tid = myThread()->id;
    // releaseLock(&lk->lk);
}

void releasesleep(struct Sleeplock* lk) {
    // acquireLock(&lk->lk);
    lk->locked = 0;
    lk->tid = 0;
    wakeup(lk);
    // releaseLock(&lk->lk);
}

int holdingsleep(struct Sleeplock* lk) {
    int r;

    // acquireLock(&lk->lk);
    r = lk->locked && (lk->tid == myThread()->id);
    // releaseLock(&lk->lk);
    return r;
}