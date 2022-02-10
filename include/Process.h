#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <Type.h>
#include <Queue.h>

#define LOG_PROCESS_NUM 10
#define PROCESS_TOTAL_NUMBER (1 << LOG_PROCESS_NUM)

#define PROCESS_CREATE_PRIORITY(x, y) { \
    extern u8 binary_##x##_start[]; \
    extern int binary_##x##_size; \
    processCreatePriority(binary_##x##_start, binary_##x##_size, y); \
}

typedef struct Trapframe {
    u64 kernelSatp;
    u64 kernelSp;
    u64 trapHandler;
    u64 epc;
    u64 kernelHartId;
    u64 ra;
    u64 sp;
    u64 gp;
    u64 tp;
    u64 t0;
    u64 t1;
    u64 t2;
    u64 s0;
    u64 s1;
    u64 a0;
    u64 a1;
    u64 a2;
    u64 a3;
    u64 a4;
    u64 a5;
    u64 a6;
    u64 a7;
    u64 s2;
    u64 s3;
    u64 s4;
    u64 s5;
    u64 s6;
    u64 s7;
    u64 s8;
    u64 s9;
    u64 s10;
    u64 s11;
    u64 t3;
    u64 t4;
    u64 t5;
    u64 t6;
} Trapframe;

enum ProcessState {
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

typedef struct Process {
    Trapframe trapframe;
    LIST_ENTRY(Process) link;
    u64 *pgdir;
    u32 id;
    u32 parentId;
    LIST_ENTRY(Process) scheduleLink;
    u32 priority;
    enum ProcessState state;
} Process;

LIST_HEAD(ProcessList, Process);

void processInit();
void processCreatePriority(u8 *binary, u32 size, u32 priority);
void wakeup(void *channel);
void yield();

#endif