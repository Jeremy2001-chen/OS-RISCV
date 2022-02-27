#include <Syscall.h>
#include <Driver.h>
#include <Process.h>

void (*syscallVector[])(void) = {
    [SYSCALL_PUTCHAR]           syscallPutchar,
    [SYSCALL_GET_PROCESS_ID]    syscallGetProcessId,
    [SYSCALL_YIELD]             syscallYield,
    [SYSCALL_FORK]              syscallFork,
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
}