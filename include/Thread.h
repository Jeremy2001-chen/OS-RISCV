#ifndef _Thread_H_
#define _Thread_H_

#include <Signal.h>
#include <Process.h>
#include <fat.h>

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
    u64 retValue;
    SignalSet blocked;
    SignalSet pending;
    u64 setChildTid;
    u64 clearChildTid;
    struct Process* process;
    u64 robustHeadPointer;
} Thread;

LIST_HEAD(ThreadList, Thread);

Thread* myThread(); // Get current running thread in this hart
void threadFree(Thread *th);
u64 getThreadTopSp(Thread* th);
SignalAction *getSignalHandler(Thread* th);

void threadInit();
int mainThreadAlloc(Thread **new, u64 parentId);
int threadAlloc(Thread **new, Process* process, u64 userSp);
int tid2Thread(u32 threadId, struct Thread **thread, int checkPerm);
void threadRun(Thread* thread);
void threadDestroy(Thread* thread);

#define NORMAL         0
#define KERNEL_GIVE_UP 1

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