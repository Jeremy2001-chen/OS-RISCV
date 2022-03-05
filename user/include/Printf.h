#ifndef _USER_PRINTF_H_
#define _USER_PRINTF_H_

#include <Syscall.h>
#include "../../include/Type.h"

#define 	LP_MAX_BUF 100

static inline void uPrintString(const char* s) {
    while (*s) {
        if (*s == '\n')
            syscallPutchar('\r');
        syscallPutchar(*s++);
    }
}

static char digits[] = "0123456789abcdef";

static inline void uPrintInt(i64 xx, int base, bool sign) {
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
        syscallPutchar(buf[i]);
    }
}

void uPrintf(const char *fmt, ...);
void _uPanic_(const char *file, int line, const char *fmt, ...);
#define uPanic(...) _uPanic_(__FILE__, __LINE__, __VA_ARGS__)

#endif 
