#ifndef __SPINLOCK_H
#define __SPINLOCK_H

// Mutual exclusion lock.
struct Spinlock {
    uint locked;       // Is the lock held?

    // For debugging:
    char *name;        // Name of lock.
    int cpu;   // The cpu holding the lock.
};

// Initialize a spinlock
void initLock(struct Spinlock*, char*);

// Acquire the spinlock
// Must be used with release()
void acquire(struct spinlock*);

// Release the spinlock
// Must be used with acquire()
void release(struct spinlock*);

// Check whether this cpu is holding the lock
// Interrupts must be off
int holding(struct spinlock*);

#endif
