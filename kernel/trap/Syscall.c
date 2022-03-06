#include <Syscall.h>
#include <Driver.h>
#include <Process.h>
#include <Type.h>
#include <Page.h>
#include <Riscv.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_GET_PROCESS_ID]    syscallGetProcessId,
    [SYSCALL_YIELD]             syscallYield,
    [SYSCALL_FORK]              syscallFork,
    [SYSCALL_PUT_STRING]        syscallPutString
};

extern Trapframe trapframe[];
void syscallPutchar() {
    putchar(trapframe->a0);
}

void syscallGetProcessId() {

}

void syscallYield() {

}

void syscallFork() {
    processFork();
}

void syscallPutString() {
    u64 va = trapframe->a0;
    int len = trapframe->a1;
    extern Process *currentProcess[HART_TOTAL_NUMBER];
    int hartId = r_hartid();
    u64* pte;
    u64 pa = pageLookup(currentProcess[hartId]->pgdir, va, &pte);
    if (pa == 0) {
        panic("Syscall put string address error!\nThe virtual address is %x, the length is %x\n", va, len);
    }
    char* start = (char*) pa;
    while (len--) {
        putchar(*start);
        start++;
    }
}