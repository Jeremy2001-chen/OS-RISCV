#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <Type.h>
#include <Queue.h>
#include <Spinlock.h>
#include <fat.h>
#include <Timer.h>
#include <file.h>
#include <Signal.h>
#include <Resource.h>
#include <exec.h>

#define NOFILE 1024  //Number of fds that a process can open
#define LOG_PROCESS_NUM 10
#define PROCESS_TOTAL_NUMBER (1 << LOG_PROCESS_NUM)
#define PROCESS_OFFSET(processId) ((processId) & (PROCESS_TOTAL_NUMBER - 1))

#define PROCESS_CREATE_PRIORITY(x, y) { \
    extern u8 binary##x##Start[]; \
    extern int binary##x##Size; \
    processCreatePriority(binary##x##Start, binary##x##Size, y); \
}

typedef struct SignalAction SignalAction;

enum ProcessState {
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

struct ProcessTime {
    long lastUserTime;
    long lastKernelTime;
};

typedef struct Process {
    // Trapframe trapframe;
    struct ProcessTime processTime;
    CpuTimes cpuTime;
    LIST_ENTRY(Process) link;
    // u64 awakeTime;
    u64 *pgdir;
    u32 processId;
    u32 parentId;
    // LIST_ENTRY(Process) scheduleLink;
    u32 priority;
    enum ProcessState state;
    struct Spinlock lock;
    // struct dirent *cwd;           // Current directory
    struct File *ofile[NOFILE];
    // u64 chan;//wait Object
    // u64 currentKernelSp;
    // int reason;
    u32 retValue;
    u64 heapBottom;
    struct dirent *execFile;
    struct dirent *cwd;
    // SignalSet blocked;
    // SignalSet pending;
    // u64 setChildTid;
    // u64 clearChildTid;
    int threadCount;
    struct ResourceLimit fileDescription;
    ProcessSegmentMap *segmentMapHead;
} Process;

LIST_HEAD(ProcessList, Process);

#define PROCESS_FORK 17
#define CSIGNAL		0x000000ff
#define CLONE_VM	0x00000100
#define CLONE_FS	0x00000200
#define CLONE_FILES	0x00000400
#define CLONE_SIGHAND	0x00000800
#define CLONE_PIDFD	0x00001000
#define CLONE_PTRACE	0x00002000
#define CLONE_VFORK	0x00004000
#define CLONE_PARENT	0x00008000
#define CLONE_THREAD	0x00010000
#define CLONE_NEWNS	0x00020000
#define CLONE_SYSVSEM	0x00040000
#define CLONE_SETTLS	0x00080000
#define CLONE_PARENT_SETTID	0x00100000
#define CLONE_CHILD_CLEARTID	0x00200000
#define CLONE_DETACHED	0x00400000
#define CLONE_UNTRACED	0x00800000
#define CLONE_CHILD_SETTID	0x01000000
#define CLONE_NEWCGROUP	0x02000000
#define CLONE_NEWUTS	0x04000000
#define CLONE_NEWIPC	0x08000000
#define CLONE_NEWUSER	0x10000000
#define CLONE_NEWPID	0x20000000
#define CLONE_NEWNET	0x40000000
#define CLONE_IO	0x80000000

Process* myProcess();
int processAlloc(Process **new, u64 parentId);
void processInit();
void processCreatePriority(u8* binary, u32 size, u32 priority);
void sleep(void* chan, struct Spinlock* lk);
void wakeup(void* channel);
void yield();
SignalAction *getSignalHandler(Process* p);
void processDestory(Process* p);
void processFree(Process* p);
int pid2Process(u32 processId, struct Process **process, int checkPerm);
int either_copyout(int user_dst, u64 dst, void* src, u64 len);
int either_copyin(void* dst, int user_src, u64 src, u64 len);
int either_memset(bool user, u64 dst, u8 value, u64 len);
int wait(int, u64, int);
int setup(Process *p);
void kernelProcessCpuTimeBegin(void);
void kernelProcessCpuTimeEnd(void);
#endif