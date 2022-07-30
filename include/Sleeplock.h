#ifndef __SLEEPLOCK_H
#define __SLEEPLOCK_H

#include "Type.h"
#include "Spinlock.h"

// Long-term locks for processes
struct Sleeplock {
  uint locked;       // Is the lock held?
  struct Spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int tid;           // Thread holding lock
};

void            acquiresleep(struct Sleeplock*);
void            releasesleep(struct Sleeplock*);
int             holdingsleep(struct Sleeplock*);
void            initsleeplock(struct Sleeplock*, char*);

#endif