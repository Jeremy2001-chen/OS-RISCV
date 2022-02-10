#include <Process.h>
#include <Page.h>
#include <Error.h>
#include <Elf.h>
#include <Riscv.h>
#include <Trap.h>

Process processes[PROCESS_TOTAL_NUMBER];
static struct ProcessList freeProcesses;
struct ProcessList scheduleList[2];
Process *currentProcess;

void processInit() {
    LIST_INIT(&freeProcesses);
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);
    int i;
    extern u64 kernelPageDirectory[];
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        processes[i].state = UNUSED;
        processes[i].trapframe.kernelSatp = (u64) kernelPageDirectory;
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);
    }
    extern char trapframe[];
    w_sscratch((u64) trapframe);
}

u32 generateProcessId(Process *p) {
    static u32 nextId = 0;
    u32 idx = p - processes;
    return (++nextId << (1 + LOG_PROCESS_NUM)) | idx;
}

extern void userVector();
static int setup(Process *p) {
    int r;
    PhysicalPage *page;
    r = pageAlloc(&page);
    if (r < 0) {
        panic("setup page alloc error\n");
        return r;
    }

    page->ref++;
    p->pgdir = (u64*) page2pa(page);
    
    extern char textEnd[];
    pageInsert(p->pgdir, (u64)userVector, (u64)textEnd, PTE_EXECUTE | PTE_READ | PTE_WRITE);
    return 0;
}

int processAlloc(Process **new, u64 parentId) {
    int r;
    Process *p;
    if (LIST_EMPTY(&freeProcesses)) {
        *new = NULL;
        return -NO_FREE_PROCESS;
    }
    p = LIST_FIRST(&freeProcesses);
    LIST_REMOVE(p, link);
    if ((r = setup(p)) < 0) {
        return r;
    }

    p->id = generateProcessId(p);
    p->state = RUNNABLE;
    p->parentId = parentId;
    extern char kernelStack[];
    p->trapframe.kernelSp = (u64)kernelStack + KERNEL_STACK_SIZE;
    p->trapframe.sp = USER_STACK_TOP;

    *new = p;
    return 0;
}

int codeMapper(u64 va, u32 segmentSize, u8 *binary, u32 binSize, void *userData) {
    Process *env = (Process*)userData;
    PhysicalPage *p = NULL;
    u64 i;
    int r = 0;
    u64 offset = va - DOWN_ALIGN(va, PAGE_SIZE);
    u64* j;

    if (offset > 0) {
        p = pa2page(pageLookup(env->pgdir, va, &j));
        if (p == NULL) {
            if (pageAlloc(&p) < 0) {
                return -1;
            }
            pageInsert(env->pgdir, va, page2pa(p), PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(binSize, PAGE_SIZE - offset);
        bcopy(binary, (void*) page2pa(p) + offset, r);
    }
    for (i = r; i < binSize; i += r) {
        if (pageAlloc(&p) != 0) {
            return -1;
        }
        pageInsert(env->pgdir, va + i, page2pa(p), PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, binSize - i);
        bcopy(binary + i, (void*) page2pa(p), r);
    }

    offset = va + i - DOWN_ALIGN(va + i, PAGE_SIZE);
    if (offset > 0) {
        p = pa2page(pageLookup(env->pgdir, va + i, &j));
        if (p == NULL) {
            if (pageAlloc(&p) != 0) {
                return -1;
            }
            pageInsert(env->pgdir, va + i, page2pa(p), PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(segmentSize - i, PAGE_SIZE - offset);
        bzero((void*) page2pa(p) + offset, r);
    }
    for (i += r; i < segmentSize; i += r) {
        if (pageAlloc(&p) != 0) {
            return -1;
        }
        pageInsert(env->pgdir, va + i, page2pa(p), PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, segmentSize - i);
        bzero((void*) page2pa(p), r);
    }
    return 0;
}

void processCreatePriority(u8 *binary, u32 size, u32 priority) {
    Process *p;

    int r = processAlloc(&p, 0);
    if (r < 0) {
        return;
    }
    p->priority = priority;
    u64 entryPoint;
    if (loadElf(binary, size, &entryPoint, p, codeMapper) < 0) {
        panic("process create error\n");
    }

    LIST_INSERT_TAIL(&scheduleList[0], p, scheduleLink);
}

void processRun(Process *p) {
    intr_off();
    extern char trapframe[];
    if (currentProcess) {
        bcopy(trapframe, &(currentProcess->trapframe), sizeof(Trapframe));
    }
    currentProcess = p;
    bcopy(&(p->trapframe), trapframe, sizeof(Trapframe));
    userTrapReturn();
}

void wakeup(void *channel) {
    // todo
}

void yield() {
    printf("process yield\n");
    static int count = 0;
    static int point = 0;
    Process* next_env = currentProcess;
    while ((count == 0) || (next_env == NULL) || (next_env->state != RUNNABLE)) {
        if (next_env != NULL) {
            LIST_INSERT_TAIL(&scheduleList[point ^ 1], next_env, scheduleLink);
        }
        if (LIST_EMPTY(&scheduleList[point])) {
            point = 1 - point;
        }
        if (LIST_EMPTY(&scheduleList[point])) {
            panic("No Env is RUNNABLE\n");
        }
        next_env = LIST_FIRST(&scheduleList[point]);
        LIST_REMOVE(next_env, scheduleLink);
        count = next_env->priority;
    }
    count--;
    processRun(next_env);
}