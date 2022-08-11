#ifndef _TYPE_H_
#define _TYPE_H_

typedef unsigned __attribute__((__mode__(QI))) u8;
typedef unsigned __attribute__((__mode__(HI))) u16;
typedef unsigned __attribute__((__mode__(SI))) u32;
typedef unsigned __attribute__((__mode__(DI))) u64;
typedef int __attribute__((__mode__(QI))) i8;
typedef int __attribute__((__mode__(HI))) i16;
typedef int __attribute__((__mode__(SI))) i32;
typedef int __attribute__((__mode__(DI))) i64;

typedef u8 bool;
#define true 1
#define false 0

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define MIN(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a <= __b ? __a : __b; \
    })

#define MAX(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a >= __b ? __a : __b; \
    })

inline int LOW_BIT64(u64 x) {
    int res = 0;
    if ((x & ((1UL << 32) - 1)) == 0) {
        x >>= 32;
        res += 32;
    }
    if ((x & ((1UL << 16) - 1)) == 0) {
        x >>= 16;
        res += 16;
    }
    if ((x & ((1UL << 8) - 1)) == 0) {
        x >>= 8;
        res += 8;
    }
    if ((x & ((1UL << 4) - 1)) == 0) {
        x >>= 4;
        res += 4;
    }
    if ((x & ((1UL << 2) - 1)) == 0) {
        x >>= 2;
        res += 2;
    }
    if ((x & ((1UL << 1) - 1)) == 0) {
        x >>= 1;
        res += 1;
    }
    return x ? res : -1;
}

typedef unsigned char uchar;
typedef unsigned short wchar;

typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define LOWBIT(x) ((x) & (-x))

#endif