#ifndef _Thread_H_
#define _Thread_H_

#include <Signal.h>
#include <Process.h>
#include <fat.h>

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
    u64 ft0;
    u64 ft1;
    u64 ft2;
    u64 ft3;
    u64 ft4;
    u64 ft5;
    u64 ft6;
    u64 ft7;
    u64 fs0;
    u64 fs1;
    u64 fa0;
    u64 fa1;
    u64 fa2;
    u64 fa3;
    u64 fa4;
    u64 fa5;
    u64 fa6;
    u64 fa7;
    u64 fs2;
    u64 fs3;
    u64 fs4;
    u64 fs5;
    u64 fs6;
    u64 fs7;
    u64 fs8;
    u64 fs9;
    u64 fs10;
    u64 fs11;
    u64 ft8;
    u64 ft9;
    u64 ft10;
    u64 ft11;
} Trapframe;

typedef struct SignalContext {
    Trapframe contextRecover;
	bool start;
	u8 signal;
    ucontext* uContext;
	LIST_ENTRY(SignalContext) link;
} SignalContext;

typedef struct Thread {
    Trapframe trapframe;
    LIST_ENTRY(Thread) link;
    u64 awakeTime;
    u32 id;
    LIST_ENTRY(Thread) scheduleLink;
    enum ProcessState state;
    struct Spinlock lock;
    u64 chan;//wait Object
    u64 currentKernelSp;
    int reason;
    u32 retValue;
    SignalSet blocked;
    SignalSet pending;
    SignalSet processing;
    u64 setChildTid;
    u64 clearChildTid;
    struct Process* process;
    u64 robustHeadPointer;
    bool killed;
	struct SignalContextList waitingSignal;
} Thread;

LIST_HEAD(ThreadList, Thread);

Thread* myThread(); // Get current running thread in this hart
void threadFree(Thread *th);
u64 getThreadTopSp(Thread* th);

void threadInit();
int mainThreadAlloc(Thread **new, u64 parentId);
int threadAlloc(Thread **new, Process* process, u64 userSp);
int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm);
void threadRun(Thread* thread);
void threadDestroy(Thread* thread);

#define NORMAL         0
#define KERNEL_GIVE_UP 1
#define SELECT_BLOCK   2

#define LOCALE_NAME_MAX 23

struct __locale_map {
	const void *map;
	u64 map_size;
	char name[LOCALE_NAME_MAX+1];
	const struct __locale_map *next;
};

struct __locale_struct {
	const struct __locale_map *cat[6];
};

struct pthread {
	/* Part 1 -- these fields may be external or
	 * internal (accessed via asm) ABI. Do not change. */
	struct pthread *self;
	u64 *dtv;
	struct pthread *prev, *next; /* non-ABI */
	u64 sysinfo;
	u64 canary, canary2;

	/* Part 2 -- implementation details, non-ABI. */
	int tid;
	int errno_val;
	volatile int detach_state;
	volatile int cancel;
	volatile unsigned char canceldisable, cancelasync;
	unsigned char tsd_used:1;
	unsigned char dlerror_flag:1;
	unsigned char *map_base;
	u64 map_size;
	void *stack;
	u64 stack_size;
	u64 guard_size;
	void *result;
	struct __ptcb *cancelbuf;
	void **tsd;
	struct {
		volatile void *volatile head;
		long off;
		volatile void *volatile pending;
	} robust_list;
	volatile int timer_id;
	struct __locale_struct *locale;
	volatile int killlock[1];
	char *dlerror_buf;
	void *stdio_locks;

	/* Part 3 -- the positions of these fields relative to
	 * the end of the structure is external and internal ABI. */
	u64 canary_at_end;
	u64 *dtv_copy;
};

#endif