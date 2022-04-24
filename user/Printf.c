#include "./include/SyscallLib.h"
#include "../include/Type.h"
#include "Printf.h"

static void uPrint(const char *fmt, va_list ap) {
    int i, c;
    char *s;

    for (i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            if (fmt[i] == '\n') {
                addCharToBuffer('\r');
            }
            addCharToBuffer(fmt[i]);
            continue;
        }
        c = fmt[++i];
        bool l = false;
        if (c == 'l') {
            l = true;
            c = fmt[++i];
        }
        switch (c) {
            case 'd':
                if (l) {
                    uPrintInt(va_arg(ap, i64), 10, true);
                } else {
                    uPrintInt((i64) va_arg(ap, i32), 10, true);
                }
                break;
            case 'x':
                if (l) {
                    uPrintInt(va_arg(ap, u64), 16, false);
                } else {
                    uPrintInt((u64) va_arg(ap, u32), 16, false);
                }
                break;
            case 's':
                if ((s = va_arg(ap, char*)) == 0) {
                    s = "(null)";
                }
                uPrintString(s);
                break;
            case '%':
                addCharToBuffer('%');
                break;
            default:
                addCharToBuffer('%');
                addCharToBuffer(c);
                break;
        }

    }
}

void printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    uPrint(fmt, ap);
    va_end(ap);
    clearBuffer();
}

void _uPanic_(const char *file, int line, const char *fmt, ...) {
    printf("user panic at %s: %d: ", file, line);
    va_list ap;
    va_start(ap, fmt);
    uPrint(fmt, ap);
    va_end(ap);
    putchar('\n');
    while (true);
}
