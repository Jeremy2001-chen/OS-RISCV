#include <Driver.h>
#include <Riscv.h>
#include <Spinlock.h>
#include <Timer.h>

struct Spinlock printLock;

inline void printLockInit(void) {
    initLock(&printLock, "printLock");
}

static inline void printString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            putchar('\r');
        }
        putchar(*s++);
    }
}

static char digits[] = "0123456789abcdef";

static inline void printInt(i64 xx, int base, bool sign) {
    char buf[30];
    int i;
    u64 x;

    if (sign && (sign = xx < 0)) {
        x = -xx;
    } else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    if (sign) {
        buf[i++] = '-';
    }

    while (--i >= 0) {
        putchar(buf[i]);
    }
}

static void print(const char *fmt, va_list ap) {
    int i, c;
    char *s;

    for (i = 0; fmt[i]; i++) {
        if (fmt[i] != '%'){
            if (fmt[i] == '\n') {
                putchar('\r');
            }
            putchar(fmt[i]);
            continue;
        }
        c = fmt[++i];
        bool l = false;
        if (c == 'l') {
            l = true;
            c = fmt[++i];
        }
        switch (c) {
            case 'c':
                putchar(va_arg(ap, u32));
                break;
            case 'd':
                if (l) {
                    printInt(va_arg(ap, i64), 10, true);
                } else {
                    printInt((i64) va_arg(ap, i32), 10, true);
                }
                break;
            case 'x':
                if (l) {
                    printInt(va_arg(ap, u64), 16, false);
                } else {
                    printInt((u64) va_arg(ap, u32), 16, false);
                }
                break;
            case 's':
                if ((s = va_arg(ap, char*)) == 0) {
                    s = "(null)";
                }
                printString(s);
                break;
            case '%':
                putchar('%');
                break;
            default:
                putchar('%');
                putchar(c);
                break;
        }
    }
}

void printf(const char *fmt, ...) {
    acquireLock(&printLock);
    // printString("[hart is : ");
    // printInt(r_hartid(), 10, 0);
    // printString("]");
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
    releaseLock(&printLock);
}

void panicPrintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
}

void _panic_(const char *file, int line, const char *func,const char *fmt, ...) {
    acquireLock(&printLock);
    panicPrintf("hartId %d panic at %s: %d in %s(): ", r_hartid(), file, line, func);
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
    putchar('\n');
    releaseLock(&printLock);
    //timerTick();
    //w_sstatus(r_sstatus() | SSTATUS_SIE);
    while (true);
}

void _assert_(const char* file, int line, const char *func, int statement) {
    if (!statement) {
        _panic_(file, line, func, "assert failed\n");
    }
}
