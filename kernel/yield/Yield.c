// Thread level yield

#include <Riscv.h>
#include <Thread.h>
#include <Page.h>
#include <Signal.h>
#include <Futex.h>

extern struct Spinlock scheduleListLock;
extern struct ThreadList scheduleList[2];

static int processTimeCount[HART_TOTAL_NUMBER] = {0};
static int processBelongList[HART_TOTAL_NUMBER] = {0};

void yield() {
    int hartId = r_hartid();
    int count = processTimeCount[hartId];
    int point = processBelongList[hartId];
    struct Thread* thread = myThread(); 
    acquireLock(&scheduleListLock);
    if (thread && thread->state == RUNNING) {
        if (thread->reason & KERNEL_GIVE_UP) {
            bcopy(getHartTrapFrame(), &thread->trapframe, sizeof(Trapframe));
        }
        thread->state = RUNNABLE;
    }
    while ((count == 0) || !thread || (thread->state != RUNNABLE) || thread->awakeTime > r_time()) {
        if (thread)
            LIST_INSERT_TAIL(&scheduleList[point ^ 1], thread, scheduleLink);
        if (LIST_EMPTY(&scheduleList[point]))
            point ^= 1;
        if (!(LIST_EMPTY(&scheduleList[point]))) {
            thread = LIST_FIRST(&scheduleList[point]);
            LIST_REMOVE(thread, scheduleLink);
            count = 1;
        }
        releaseLock(&scheduleListLock);
        acquireLock(&scheduleListLock);
    }
    releaseLock(&scheduleListLock);
    count--;
    processTimeCount[hartId] = count;
    processBelongList[hartId] = point;
    // printf("hartID %d yield thread %lx, the process is %lx\n", hartId, thread->id, thread->process->processId);
    if (thread->awakeTime > 0) {
        getHartTrapFrame()->a0 = 0;
        thread->awakeTime = 0;
    }
    futexClear(thread);
    threadRun(thread);
}