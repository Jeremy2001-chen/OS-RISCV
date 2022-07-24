#include <Futex.h>
#include <Process.h>
#include <Thread.h>

typedef struct FutexQueue
{
    u64 addr;
    Thread* thread;
    u8 valid;
} FutexQueue;

FutexQueue futexQueue[FUTEX_COUNT];

void futexWait(u64 addr, Thread* th) {
    for (int i = 0; i < FUTEX_COUNT; i++) {
        if (!futexQueue[i].valid) {
            futexQueue[i].valid = true;
            futexQueue[i].addr = addr;
            futexQueue[i].thread = th;
            th->state = SLEEPING;
            yield();
            // not reach here!!!
        }
    }
    panic("No futex Resource!\n");
}

void futexWake(u64 addr, int n) {
    for (int i = 0; i < FUTEX_COUNT && n; i++) {
        if (futexQueue[i].valid && futexQueue[i].addr == addr) {
            futexQueue[i].thread->state = RUNNABLE;
            futexQueue[i].thread->trapframe.a0 = 0; // set next yield accept!
            futexQueue[i].valid = false;
            n--;
        }
    }
    yield();
}