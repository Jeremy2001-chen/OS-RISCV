#include <Process.h>
#include <Type.h>
#include <Sysarg.h>
#include <Driver.h>
#include <string.h>
#include <Page.h>

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.

int fetchstr(u64 addr, char* buf, int max) {
    struct Process* p = myproc();
    int err = copyinstr(p->pgdir, buf, addr, max);
    if (err < 0)
        return err;
    return strlen(buf);
}


static u64 argraw(int n) {
    Trapframe *trapframe = getHartTrapFrame();
    switch (n) {
        case 0:
            return trapframe->a0;
        case 1:
            return trapframe->a1;
        case 2:
            return trapframe->a2;
        case 3:
            return trapframe->a3;
        case 4:
            return trapframe->a4;
        case 5:
            return trapframe->a5;
    }
    panic("argraw");
    return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int* ip) {
    *ip = argraw(n);
    return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int argaddr(int n, u64* ip) {
    *ip = argraw(n);
    return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.

int argstr(int n, char* buf, int max) {
    u64 addr;
    if (argaddr(n, &addr) < 0)
        return -1;
    return fetchstr(addr, buf, max);
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int copyinstr(u64* pagetable, char* dst, u64 srcva, u64 max) {
    u64 n, va0, pa0;
    int got_null = 0;

    while (got_null == 0 && max > 0) {
        va0 = DOWN_ALIGN(srcva, PGSIZE);
        pa0 = vir2phy(pagetable, va0);
        if (pa0 == 0){
            printf("pa0=0!");
            return -1;
        }
        n = PGSIZE - (srcva - va0);
        if (n > max)
            n = max;

        char* p = (char*)(pa0 + (srcva - va0));
        while (n > 0) {
            if (*p == '\0') {
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }

        srcva = va0 + PGSIZE;
    }
    if (got_null) {
        return 0;
    } else {
        printf("ungot null\n");
        return -1;
    }
}
