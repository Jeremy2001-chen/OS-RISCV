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

#define MIN(_a, _b) \
    ({  \
        typeof(_a) __a = (_a); \
        typeof(_b) __b = (_b); \
        __a <= __b ? __a : __b; \
    })

#endif