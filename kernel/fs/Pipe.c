#include <Debug.h>
#include <pipe.h>
#include "Page.h"
#include "Process.h"
#include "Sleeplock.h"
#include "Spinlock.h"
#include "Type.h"
#include <file.h>
#include "string.h"
#include "Riscv.h"

struct pipe pipeBuffer[MAX_PIPE];
u64 pipeBitMap[MAX_PIPE / 64];

int pipeAlloc(struct pipe** p) {
    for (int i = 0; i < MAX_PIPE / 64; i++) {
        if (~pipeBitMap[i]) {
            int bit = LOW_BIT64(~pipeBitMap[i]);
            pipeBitMap[i] |= (1UL << bit);
            *p = &pipeBuffer[(i << 6) | bit];
            (*p)->nread = (*p)->nwrite = 0;
            return 0;
        }
    }
    panic("no pipe!");
}

void pipeFree(struct pipe* p) {
    int off = p - pipeBuffer;
    pipeBitMap[off >> 6] &= ~(1UL << (off & 63));
}

int pipeNew(struct File** f0, struct File** f1) {
    struct pipe* pi;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    pipeAlloc(&pi);
    pi->readopen = 1;
    pi->writeopen = 1;
    initLock(&pi->lock, "pipe");
    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;
    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    return 0;

bad:
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

void pipeClose(struct pipe* pi, int writable) {
    acquireLock(&pi->lock);
    // printf("%x %x %x\n", pi->writeopen, pi->readopen, writable);
    if (writable) {
        pi->writeopen = 0;
        wakeup(&pi->nread);
    } else {
        pi->readopen = 0;
        wakeup(&pi->nwrite);
    }
    if (pi->readopen == 0 && pi->writeopen == 0) {
        releaseLock(&pi->lock);
        pipeFree(pi);
    } else
        releaseLock(&pi->lock);
}

int pipeWrite(struct pipe* pi, bool isUser, u64 addr, int n) {
    int i = 0;
    
    // printf("%s %d\n", __FILE__, __LINE__);

    // printf("%d WWWW pipe addr %x\n", r_hartid(), pi);
    acquireLock(&pi->lock);
    while (i < n) {
        // printf("i=%d n=%d\n", i, n);
        // printf("hart id %x, now write %d, to %d, %d\n", r_hartid(), i, n, pi->readopen);
        if (pi->readopen == 0 /*|| pr->killed*/) {
            releaseLock(&pi->lock);
            panic("");
            return -1;
        }
        if (pi->nwrite == pi->nread + PIPESIZE) {  // DOC: pipewrite-full
            wakeup(&pi->nread);
            // printf("Write %x Sleep? Now %x for %x, start %x, end %x, ask for %x\n", r_hartid(), i, n, pi->nread, pi->nwrite, &pi->nwrite);
            // printf("Write %lx Start sleep\n", myProcess()->processId);
            sleep(&pi->nwrite, &pi->lock);
            // printf("Write %x Stop sleep\n", r_hartid());
        } else {
            // printf("%d %d\n", i, n);
            char ch;
            if (either_copyin(&ch, isUser, addr + i, 1) == -1) {
                // printf("%s %d\n", __FILE__, __LINE__);
                break;
            }
            pi->data[(pi->nwrite++) & (PIPESIZE - 1)] = ch;
            i++;
        }
    }
    wakeup(&pi->nread);
    // printf("%d %d\n", pi->nread, pi->nwrite);
    releaseLock(&pi->lock);
    // printf("%s %d %d\n", __FILE__, __LINE__, i);
    assert(i != 0);
    return i;
}

int pipeRead(struct pipe* pi, bool isUser, u64 addr, int n) {
    int i;
    char ch;
    // printf("[read] pipe:%lx nread: %d nwrite: %d\n", pi, pi->nread, pi->nwrite);

    // printf("%d RRRR pipe addr %x\n", r_hartid(), pi);
    acquireLock(&pi->lock);
    while (pi->nread == pi->nwrite && pi->writeopen) {  // DOC: pipe-empty
        if (0 /*pr->killed*/) {
            releaseLock(&pi->lock);
            return -1;
        }
        // printf("Read %lx Start Sleep\n", myProcess()->processId);
        sleep(&pi->nread, &pi->lock);  // DOC: piperead-sleep
        // printf("Read %lx Stop sleep\n", myProcess()->processId);
    }
    // printf("%s %d %lx %lx\n", __FILE__, __LINE__, pi->nread, pi->nwrite); 
    for (i = 0; i < n; i++) {  // DOC: piperead-copy
        if (pi->nread == pi->nwrite) {
            break;
        }
        // printf("hart id %x, now read %d\n", r_hartid(), i);
        ch = pi->data[(pi->nread++) & (PIPESIZE - 1)];
        // printf("%x %x\n", r_hartid(), ch);
        if (either_copyout(isUser, addr + i, &ch, 1) == -1) {
            break;
        }
    }
    // printf("%x wake up %x, start %x, end %x\n", r_hartid(), &pi->nwrite, pi->nread, pi->nwrite);
    wakeup(&pi->nwrite);  // DOC: piperead-wakeup
    releaseLock(&pi->lock);
    return i;
}