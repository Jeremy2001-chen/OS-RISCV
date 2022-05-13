#ifndef __DEBUG_H
#define __DEBUG_H

#include "defs.h"
//if we undefine "DEBUG", some variable become unused, so we will get
//a compile error.
//so we can use use() to cheat the compiler
inline void use(void* x) {}

#ifdef ZZY_DEBUG

// print a format string
#define MSG_PRINT(...) printf("[debug]in %s() ", __func__);printf(__VA_ARGS__);printf("\n")
// print a const char * variable
#define STR_PRINT(val) printf("[debug]in %s() " #val "=%s\n", __func__, val)
// print a variable in form of dec
#define DEC_PRINT(val) printf("[debug]in %s() " #val "=%d\n", __func__, val)
// print a variable in form of hex
#define HEX_PRINT(val) printf("[debug]in %s() " #val "=0x%lx\n", __func__, val)

#else

#define MSG_PRINT(...)
#define STR_PRINT(val)
#define DEC_PRINT(val)
#define HEX_PRINT(val)

#endif

#endif