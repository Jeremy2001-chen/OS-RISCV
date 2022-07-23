#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <Type.h>
#include <Timer.h>

#define SIGNAL_COUNT 64
typedef u64 SignalSet;

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
    void (*sa_sigaction)(int, SignalInfo *, void *);
    SignalSet sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
} SignalAction;

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

struct Process;

int signProccessMask(u64, SignalSet*);
int doSignalAction(int sig, u64 act, u64 oldAction);
int doSignalTimedWait(SignalSet *which, SignalInfo *info, TimeSpec *ts);
void handleSignal(struct Process* process);

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
