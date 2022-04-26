#ifndef _USER_PRINTF_H_
#define _USER_PRINTF_H_

#include "SyscallLib.h"
#include "../../include/Type.h"

#define 	BUFFER_MAX_LEN 128

extern char printfBuffer[BUFFER_MAX_LEN];
extern int bufferLen;

static inline void clearBuffer() {
    if (bufferLen > 0) {
        putString(printfBuffer, bufferLen);
        bufferLen = 0;
    }
}

static inline void addCharToBuffer(char c) {
    printfBuffer[bufferLen++] = c;
    if (bufferLen == BUFFER_MAX_LEN) {
        clearBuffer();
    }
}

static inline void uPrintString(const char* s) {
    while (*s) {
        if (*s == '\n')
            addCharToBuffer('\r');
        addCharToBuffer(*s++);
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
        addCharToBuffer(buf[i]);
    }
}

void printf(const char *fmt, ...);
void _uPanic_(const char *file, int line, const char *fmt, ...);
#define uPanic(...) _uPanic_(__FILE__, __LINE__, __VA_ARGS__)

#endif 