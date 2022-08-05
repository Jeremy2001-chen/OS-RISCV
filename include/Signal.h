#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <Type.h>
#include <Timer.h>
#include <Process.h>
#include <Driver.h>

// Don't include Thread.h!!!!

#define SIGNAL_COUNT 128
typedef struct SignalSet {
    u64 signal[SIGNAL_COUNT / sizeof(u64)];
} SignalSet;

inline static bool signalSetAnd(int signal, SignalSet *ss) {
    if (signal > 64) {
        panic("");
    }
    return (ss->signal[0] & (1UL << (signal - 1))) != 0;
}

inline static void signalProcessStart(int signal, SignalSet *ss) {
    if (signal > 64) {
        panic("");
    }
    ss->signal[0] |= (1UL << (signal - 1));
}

inline static void signalProcessEnd(int signal, SignalSet *ss) {
    if (signal > 64) {
        panic("");
    }
    ss->signal[0] &= ~(1UL << (signal - 1));
}

typedef struct SignalInfo {
    int signo;      /* signal number */
    int errno;      /* errno value */
    int code;       /* signal code */
    int trapno;     /* trap that caused hardware signal (unusued on most architectures) */
    u32    si_pid;        /* sending PID */
    u32    si_uid;        /* real UID of sending program */
    int      si_status;     /* exit value or signal */
    u32  si_utime;      /* user time consumed */
    u32  si_stime;      /* system time consumed */
    u32 si_value;      /* signal value */
    int      si_int;        /* POSIX.1b signal */
    void    *si_ptr;        /* POSIX.1b signal */
    int      si_overrun;    /* count of timer overrun */
    int      si_timerid;    /* timer ID */
    void    *si_addr;       /* memory location that generated fault */
    long     si_band;       /* band event */
    int      si_fd;         /* file descriptor */
    short    si_addr_lsb;   /* LSB of address */
    void    *si_lower;      /* lower bound when address vioation occured */
    void    *si_upper;      /* upper bound when address violation occured */
    int      si_pkey;       /* protection key on PTE causing faut */
    void    *si_call_addr;  /* address of system call instruction */
    int      si_syscall;    /* number of attempted syscall */
    unsigned int si_arch;   /* arch of attempted syscall */
} SignalInfo;

typedef struct SignalAction {
    void (*handler)(int);
	unsigned long flags;
	void (*restorer)(void);
	unsigned mask[2];
} SignalAction;

typedef unsigned long gregset_t[32];
struct __riscv_f_ext_state {
	unsigned int f[32];
	unsigned int fcsr;
};

struct __riscv_d_ext_state {
	unsigned long long f[32];
	unsigned int fcsr;
};

struct __riscv_q_ext_state {
	unsigned long long f[64] __attribute__((aligned(16)));
	unsigned int fcsr;
	unsigned int reserved[3];
};

union __riscv_fp_state {
	struct __riscv_f_ext_state f;
	struct __riscv_d_ext_state d;
	struct __riscv_q_ext_state q;
};

typedef union __riscv_fp_state fpregset_t;

typedef struct sigcontext {
	gregset_t gregs;
	fpregset_t fpregs;
} mcontext;

struct sigaltstack {
	void *ss_sp;
	int ss_flags;
	u64 ss_size;
};

typedef struct ucontext {
	unsigned long uc_flags;
	struct ucontext *uc_link;
	struct sigaltstack uc_stack;
	SignalSet uc_sigmask;
	mcontext uc_mcontext;
} ucontext;

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

typedef struct SignalContext SignalContext;
typedef struct Thread Thread;
LIST_HEAD(SignalContextList, SignalContext);

#define SIGNAL_CONTEXT_COUNT (1024)
void signalInit();
void signalContextFree(SignalContext* sc);
int signalContextAlloc(SignalContext **signalContext);
int signalSend(int tid, int sig);
int processSignalSend(int pid, int sig);
int signProccessMask(u64 how, SignalSet *newSet);
int doSignalAction(int sig, u64 act, u64 oldAction);
SignalContext* getFirstSignalContext(Thread* thread);
void initFrame(SignalContext* sc, Thread* thread);
void signalFinish(Thread* thread, SignalContext* sc);
void handleSignal(Thread* thread);
int doSignalTimedWait(SignalSet *which, SignalInfo *info, TimeSpec *ts);
SignalContext* getHandlingSignal(Thread* thread);

#define MC_PC gregs[0]

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31

#define SIGTIMER 32
#define SIGCANCEL 33
#define SIGSYNCCALL 34
#endif
