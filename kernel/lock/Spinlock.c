#include "Spinlock.h"

void initLock(struct Spinlock* lock, char* name) {
    lock->name = name;
    lock->locked = 0;
    lock->cpu = 0;
}

void acquire(struct Spinlock* lock) {

}