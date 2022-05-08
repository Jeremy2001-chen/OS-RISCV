#include <Riscv.h>
#include <Driver.h>
#include <Timer.h>
#include <Trap.h>
#include <Process.h>
#include <Page.h>
#include <Syscall.h>
#include <Hart.h>
#include <sysfile.h>
#include <debug.h>
#include <defs.h>
#include <exec.h>

void trapInit() {
    printf("Trap init start...\n");
    w_stvec((u64)kernelVector);
    w_sstatus(r_sstatus() | SSTATUS_SIE | SSTATUS_SPIE);
    // setNextTimeout();
    w_sip(0); //todo
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE);
    printf("Trap init finish!\n");
}

int trapDevice() {
    u64 scause = r_scause();
    #ifdef QEMU
    if ((scause & SCAUSE_INTERRUPT) && 
    ((scause & SCAUSE_EXCEPTION_CODE) == SCAUSE_SUPERVISOR_EXTERNAL)) {
    #else
    // todo
    if ((scause & SCAUSE_INTERRUPT) && 
    ((scause & SCAUSE_EXCEPTION_CODE) == SCAUSE_SUPERVISOR_EXTERNAL)) {
    #endif
        int irq = interruptServed();
        if (irq == UART_IRQ) {
            int c = getchar();
            if (c != -1) {
                consoleInterrupt(c);
            }
        } else if (irq == DISK_IRQ) {
            // todo
        } else if (irq) {
            panic("unexpected interrupt irq = %d\n", irq);
        }
        if (irq) {
            interruptCompleted(irq);
        }
        #ifndef QEMU
        // todo
        #endif
        return SOFTWARE_TRAP;
    }
    if ((scause & SCAUSE_INTERRUPT) &&
    ((scause & SCAUSE_EXCEPTION_CODE) == SCAUSE_SUPERVISOR_TIMER)) {
        timerTick();
        return TIMER_INTERRUPT;
    }
    return UNKNOWN_DEVICE;
}

void kernelTrap() {
    u64 sepc = r_sepc();
    u64 sstatus = r_sstatus();
    u64 scause = r_scause();
    u64 hartId = r_hartid();
    extern Process *currentProcess[HART_TOTAL_NUMBER];

#ifdef CJY_DEBUG
    printf("[Kernel Trap] hartId is %lx, status is %lx, spec is %lx, cause is %lx, stval is %lx\n", hartId, sstatus, sepc, scause, r_stval());
#endif

    Trapframe* trapframe = getHartTrapFrame();

    trapframeDump(trapframe);
    if (!(sstatus & SSTATUS_SPP)) {
        panic("kernel trap not from supervisor mode");
    }
    if (intr_get()) {
        panic("kernel trap while interrupts enbled");
    }
    
    int device = trapDevice();
    if (device == UNKNOWN_DEVICE) {
        u64* pte;
        int pa = pageLookup(currentProcess[hartId]->pgdir, r_stval(), &pte);
        panic("unhandled error %d,  %lx, %lx\n", scause, r_stval(), pa);
        panic("kernel trap");
    }
    if (device == TIMER_INTERRUPT) {
        yield();
    }
    w_sepc(sepc);
    w_sstatus(sstatus);
}

static inline void userProcessCpuTimeEnd() {
    Process *p = myproc();
    long currentTime = r_time();
    p->cpuTime.user += currentTime - p->processTime.lastUserTime;
}

void userTrap() {
    u64 sepc = r_sepc();
    u64 sstatus = r_sstatus();
    u64 scause = r_scause();
    u64 hartId = r_hartid();

#ifdef CJY_DEBUG
    printf("[User Trap] hartId is %lx, status is %lx, spec is %lx, cause is %lx, stval is %lx\n", hartId, sstatus, sepc, scause, r_stval());
#else
    use((void *)sepc);
#endif
    if (sstatus & SSTATUS_SPP) {
        panic("usertrap: not from user mode\n");
    }
    w_stvec((u64) kernelVector);
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    userProcessCpuTimeEnd();
    Trapframe* trapframe = getHartTrapFrame();
    if (scause & SCAUSE_INTERRUPT) {
        trapDevice();
        yield();
    } else {
        kernelProcessCpuTimeBegin();
        u64 *pte = NULL;
        u64 pa = -1;
        switch (scause & SCAUSE_EXCEPTION_CODE)
        {
        case SCAUSE_ENVIRONMENT_CALL:
            trapframe->epc += 4;
            //because some func have return value, while other func haven't return value.
            //so I use if-else to discriminate them.
            if(trapframe->a7==SYSCALL_READDIR)
                trapframe->a0 = sys_readdir();
            else if(trapframe->a7==SYSCALL_FSTAT)
                trapframe->a0 = sys_fstat();
            else if(trapframe->a7==SYSCALL_EXEC)
                trapframe->a0 = sys_exec();
            else if(trapframe->a7==SYSCALL_CWD)
                trapframe->a0 = sys_cwd();
            else
                syscallVector[trapframe->a7]();
            break;
        case SCAUSE_LOAD_PAGE_FAULT:
        case SCAUSE_STORE_PAGE_FAULT:
            pa = pageLookup(currentProcess[hartId]->pgdir, r_stval(), &pte);
            if (pa == 0) {
                pageout(currentProcess[hartId]->pgdir, r_stval());
            } else if (*pte & PTE_COW) {
                cowHandler(currentProcess[hartId]->pgdir, r_stval());
            } else {
                panic("unknown");
            }
            break;
        default:
            trapframeDump(trapframe);
            pageLookup(currentProcess[hartId]->pgdir, r_stval(), &pte);
            panic("unhandled error %d,  %lx, %lx\n", scause, r_stval(), *pte);
            break;
        }
    }
    kernelProcessCpuTimeEnd();
    userTrapReturn();
}

static inline void userProcessCpuTimeBegin() {
    Process *p = myproc();
    p->processTime.lastUserTime = r_time();
}

void userTrapReturn() {
    userProcessCpuTimeBegin();
    extern char trampoline[];
    w_stvec(TRAMPOLINE_BASE + ((u64)userVector - (u64)trampoline));
    int hartId = r_hartid();

    extern Process *currentProcess[HART_TOTAL_NUMBER];
    Trapframe* trapframe = getHartTrapFrame();

    trapframe->kernelSp = getProcessTopSp(myproc());
    trapframe->trapHandler = (u64)userTrap;
    trapframe->kernelHartId = r_tp();

    //bcopy(&(currentProcess->trapframe), trapframe, sizeof(Trapframe));

    u64 sstatus = r_sstatus();
    sstatus &= ~SSTATUS_SPP;
    sstatus |= SSTATUS_SPIE;
    w_sstatus(sstatus);
    u64 satp = MAKE_SATP(currentProcess[hartId]->pgdir);
    u64 fn = TRAMPOLINE_BASE + ((u64)userReturn - (u64)trampoline);
    u64* pte;
    u64 pa = pageLookup(currentProcess[hartId]->pgdir, USER_STACK_TOP - PAGE_SIZE, &pte);
    if (pa > 0) {
        long* tem = (long*)(pa + 4072);
#ifdef CJY_DEBUG
        printf("[OUT]hart: %d, RA: %lx\n", hartId, *tem);
#else
        //We must use 'tem', otherwise we will get compile error.
        use(tem);
#endif
    }
    ((void(*)(u64, u64))fn)((u64)trapframe, satp);
}

void trapframeDump(Trapframe *tf)
{
    printf(" a0: %lx\n \
a1: %lx\n \
a2: %lx\n \
a3: %lx\n \
a4: %lx\n \
a5: %lx\n \
a6: %lx\n \
a7: %lx\n \
t0: %lx\n \
t1: %lx\n \
t2: %lx\n \
t3: %lx\n \
t4: %lx\n \
t5: %lx\n \
t6: %lx\n \
s0: %lx\n \
s1: %lx\n \
s2: %lx\n \
s3: %lx\n \
s4: %lx\n \
s5: %lx\n \
s6: %lx\n \
s7: %lx\n \
s8: %lx\n \
s9: %lx\n \
s10: %lx\n \
s11: %lx\n \
ra: %lx\n \
sp: %lx\n \
gp: %lx\n \
tp: %lx\n \
epc: %lx\n \
kernelSp: %lx\n \
kernelSatp: %lx\n \
trapHandler: %lx\n \
kernelHartId: %lx\n",
            tf->a0, tf->a1, tf->a2, tf->a3, tf->a4,
            tf->a5, tf->a6, tf->a7, tf->t0, tf->t1,
            tf->t2, tf->t3, tf->t4, tf->t5, tf->t6,
            tf->s0, tf->s1, tf->s2, tf->s3, tf->s4,
            tf->s5, tf->s6, tf->s7, tf->s8, tf->s9,
            tf->s10, tf->s11, tf->ra, tf->sp, tf->gp,
            tf->tp, tf->epc, tf->kernelSp, tf->kernelSatp,
            tf->trapHandler, tf->kernelHartId);
}
