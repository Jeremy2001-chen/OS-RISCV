#ifndef _USER_PRINTF_H_
#define _USER_PRINTF_H_

#include "SyscallLib.h"

#define 	BUFFER_MAX_LEN 128

extern char printfBuffer[BUFFER_MAX_LEN];
extern int bufferLen;

static inline void clearBuffer() {
    if (bufferLen > 0) {
        putString(printfBuffer, bufferLen);
        bufferLen = 0;
    }
}

static inline void appendBuffer(char c) {
    printfBuffer[bufferLen++] = c;
    if (bufferLen == BUFFER_MAX_LEN) {
        clearBuffer();
    }
}

static inline void appendString(const char* s) {
    while (*s) {
        if (*s == '\n')
            appendBuffer('\r');
        appendBuffer(*s++);
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
        appendBuffer(buf[i]);
    }
}

void printChar(char *buf, char c, int length, int ladjust);
void printString(char * buf, char* s, int length, int ladjust);
void printNum(char * buf, unsigned long u, int base, int negFlag, 
	 int length, int ladjust, char padc, int upcase);
void printf(const char *fmt, ...);
void _panic_(const char *file, int line, const char *fmt, ...);
#define uPanic(...) _panic_(__FILE__, __LINE__, __VA_ARGS__)
void _assert_(const char*, int, const char *, int);
#define assert(x) _assert_(__FILE__, __LINE__, __func__, x)

#endif 
