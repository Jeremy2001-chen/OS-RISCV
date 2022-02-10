#include <Riscv.h>
#include <Driver.h>
#include <Timer.h>
#include <Trap.h>
#include <Process.h>

void trapInit() {
    w_stvec((u64)kernelVector);
    w_sstatus(r_sstatus() | SSTATUS_SIE | SSTATUS_SPIE);
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE);
    setNextTimeout();
    while (true);
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
    extern Process *current;
    if (device == TIMER_INTERRUPT && current != NULL && current->state == RUNNING) {
        yield();
    }
    w_sepc(sepc);
    w_sstatus(sstatus);
}

void userTrap() {
    printf("user trap\n");
    u64 sstatus = r_sstatus();
    if (sstatus & SSTATUS_SPP) {
        panic("usertrap: not from user mode\n");
    }
    w_stvec((u64) kernelVector);
    extern Process *currentProcess;
    currentProcess->trapframe.epc = r_sepc();
    
    u64 scause = r_scause();
    printf("scause is: %lx\n", scause);
    int device = UNKNOWN_DEVICE;
    if (scause == SCAUSE_ENVIRONMENT_CALL) {
        printf("%d\n", currentProcess->trapframe.a0);
        // todo
    } else if ((device = trapDevice()) == UNKNOWN_DEVICE) {
        panic("unexpected scause\n");
    }
    if (device == TIMER_INTERRUPT) {
        yield();
    }
}

void userTrapReturn() {
    extern char trampoline[];
    w_stvec(TRAMPOLINE_BASE + ((u64)userVector - (u64)trampoline));
    extern Process *currentProcess;
    currentProcess->trapframe.trapHandler = (u64)userTrap;
    extern char kernelStack[];
    currentProcess->trapframe.kernelSp = (u64)kernelStack + KERNEL_STACK_SIZE;
    currentProcess->trapframe.kernelHartId = r_tp();

    u64 sstatus = r_sstatus();
    sstatus &= ~SSTATUS_SPP;
    sstatus |= SSTATUS_SPIE;
    w_sstatus(sstatus);
    w_sepc(currentProcess->trapframe.epc);
    u64 satp = MAKE_SATP(currentProcess->pgdir);
    u64 fn = TRAMPOLINE_BASE + ((u64)userReturn - (u64)trampoline);
    extern char trapframe[];
    ((void(*)(u64, u64))fn)(TRAMPOLINE_BASE + trapframe - trampoline, satp);
}
