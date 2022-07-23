#ifndef _CLONE_H_
#define _CLONE_H_

#include <Type.h>

int clone(u32 flags, u64 stackVa, u64 ptid, u64 tls, u64 ctid);

#endif