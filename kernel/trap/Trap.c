#include <Riscv.h>
#include <Driver.h>
#include <Timer.h>
#include <Trap.h>
#include <Process.h>
#include <Page.h>
#include <Syscall.h>
#include <Hart.h>

void trapInit() {
    printf("Trap init start...\n");
    w_stvec((u64)kernelVector);
    w_sstatus(r_sstatus() | SSTATUS_SIE | SSTATUS_SPIE);
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE);
    setNextTimeout();
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
    printf("kernel trap\n");
    u64 sepc = r_sepc();
    u64 sstatus = r_sstatus();

    if (!(sstatus & SSTATUS_SPP)) {
        panic("kernel trap not from supervisor mode");
    }
    if (intr_get()) {
        panic("kernel trap while interrupts enbled");
    }
    
    int device = trapDevice();
    if (device == UNKNOWN_DEVICE) {
        panic("kernel trap");
    }
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    int hartId = r_hartid();
    if (device == TIMER_INTERRUPT && currentProcess[hartId] != NULL &&
        currentProcess[hartId]->state == RUNNING) {
        yield();
    }
    w_sepc(sepc);
    w_sstatus(sstatus);
}

void userTrap() {
    u64 sstatus = r_sstatus();
    if (sstatus & SSTATUS_SPP) {
        panic("usertrap: not from user mode\n");
    }
    w_stvec((u64) kernelVector);
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    int hartId = r_hartid();

    u64 scause = r_scause();
    Trapframe* trapframe = getHartTrapFrame();
    if (scause & SCAUSE_INTERRUPT) {
        trapDevice();
        yield();
    } else {
        u64 *pte = NULL;
        u64 pa = -1;
        switch (scause & SCAUSE_EXCEPTION_CODE)
        {
        case SCAUSE_ENVIRONMENT_CALL:
            trapframe->epc += 4;
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
            panic("unhandled error %d,  %lx\n", scause, r_stval());
            break;
        }
    }
    userTrapReturn();
}

void userTrapReturn() {
    extern char trampoline[];
    w_stvec(TRAMPOLINE_BASE + ((u64)userVector - (u64)trampoline));
    int hartId = r_hartid();

    extern Process *currentProcess[HART_TOTAL_NUMBER];
    Trapframe* trapframe = getHartTrapFrame();

    trapframe->kernelSp = getHartKernelTopSp();
    trapframe->trapHandler = (u64)userTrap;
    trapframe->kernelHartId = r_tp();

    //bcopy(&(currentProcess->trapframe), trapframe, sizeof(Trapframe));

    u64 sstatus = r_sstatus();
    sstatus &= ~SSTATUS_SPP;
    sstatus |= SSTATUS_SPIE;
    w_sstatus(sstatus);
    u64 satp = MAKE_SATP(currentProcess[hartId]->pgdir);
    u64 fn = TRAMPOLINE_BASE + ((u64)userReturn - (u64)trampoline);
    ((void(*)(u64, u64))fn)(TRAMPOLINE_BASE + (u64)trapframe - (u64)trampoline, satp);
}
