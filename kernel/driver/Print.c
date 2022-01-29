#include <Type.h>
#include <Driver.h>

static inline void printString(const char* s) {
    while (*s) {
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

    if(sign) {
        buf[i++] = '-';
    }

    while(--i >= 0) {
        putchar(buf[i]);
    }
}

void printf(char *fmt, ...) {
    va_list ap;
    int i, c;
    char *s;

    va_start(ap, fmt);
    for(i = 0; fmt[i]; i++) {
        if(fmt[i] != '%'){
            putchar(fmt[i]);
            continue;
        }
        c = fmt[++i];
        bool l = false;
        if(c == 'l') {
            l = true;
            c = fmt[++i];
        }
        switch(c){
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
            if((s = va_arg(ap, char*)) == 0) {
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