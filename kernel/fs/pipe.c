#include <Debug.h>
#include <pipe.h>
#include "Page.h"
#include "Process.h"
#include "Sleeplock.h"
#include "Spinlock.h"
#include "Type.h"
#include "file.h"
#include "string.h"
#include "Riscv.h"

int pipealloc(struct file** f0, struct file** f1) {
    struct pipe* pi;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    PhysicalPage* pp = NULL;
    if (pageAlloc(&pp) != 0)
        goto bad;
    pi = (struct pipe*)page2pa(pp);
    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
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
    if (pi)
        pageFree(pa2page((u64)pi));
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    return -1;
}

void pipeclose(struct pipe* pi, int writable) {
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
        pageFree(pa2page((u64)pi));
    } else
        releaseLock(&pi->lock);
}

int pipewrite(struct pipe* pi, u64 addr, int n) {
    int i = 0;
    struct Process* pr = myproc();

    // printf("%d WWWW pipe addr %x\n", r_hartid(), pi);
    acquireLock(&pi->lock);
    while (i < n) {
        // printf("hart id %x, now write %d, to %d, %d\n", r_hartid(), i, n, pi->readopen);
        if (pi->readopen == 0 /*|| pr->killed*/) {
            releaseLock(&pi->lock);
            return -1;
        }
        if (pi->nwrite == pi->nread + PIPESIZE) {  // DOC: pipewrite-full
            wakeup(&pi->nread);
            // printf("Write %x Sleep? Now %x for %x, start %x, end %x, ask for %x\n", r_hartid(), i, n, pi->nread, pi->nwrite, &pi->nwrite);
            sleep(&pi->nwrite, &pi->lock);
            // printf("Write %x Stop sleep\n", r_hartid());
        } else {
            // printf("%d %d\n", i, n);
            char ch;
            if (copyin(pr->pgdir, &ch, addr + i, 1) == -1)
                break;
            pi->data[pi->nwrite++ % PIPESIZE] = ch;
            i++;
        }
    }
    wakeup(&pi->nread);
    // printf("%d %d\n", pi->nread, pi->nwrite);
    releaseLock(&pi->lock);
    return i;
}

int piperead(struct pipe* pi, u64 addr, int n) {
    int i;
    struct Process* pr = myproc();
    char ch;

    // printf("%d RRRR pipe addr %x\n", r_hartid(), pi);
    acquireLock(&pi->lock);
    while (pi->nread == pi->nwrite && pi->writeopen) {  // DOC: pipe-empty
        if (0 /*pr->killed*/) {
            releaseLock(&pi->lock);
            return -1;
        }
        // printf("Read %x Sleep?\n", r_hartid());
        sleep(&pi->nread, &pi->lock);  // DOC: piperead-sleep
        // printf("Read %x Stop sleep\n", r_hartid());
    }
    for (i = 0; i < n; i++) {  // DOC: piperead-copy
        if (pi->nread == pi->nwrite)
            break;
        // printf("hart id %x, now read %d\n", r_hartid(), i);
        ch = pi->data[pi->nread++ % PIPESIZE];
        // printf("%x %x\n", r_hartid(), ch);
        if (copyout(pr->pgdir, addr + i, &ch, 1) == -1) {
            break;
        }
    }
    // printf("%x wake up %x, start %x, end %x\n", r_hartid(), &pi->nwrite, pi->nread, pi->nwrite);
    wakeup(&pi->nwrite);  // DOC: piperead-wakeup
    releaseLock(&pi->lock);
    return i;
}