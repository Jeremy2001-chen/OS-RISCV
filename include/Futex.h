#ifndef _FUTEX_H_
#define _FUTEX_H_

#include <Type.h>
#include <Driver.h>
#include <Process.h>
#include <Thread.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#define FUTEX_PRIVATE_FLAG 128
#define FUTEX_COUNT 128

void futexWait(u64 addr, Thread* thread);
void futexWake(u64 addr, int n);

#endif