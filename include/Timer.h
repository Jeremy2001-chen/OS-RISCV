#ifndef _TIMER_H_
#define _TIMER_H_

#define INTERVAL 200000
#include "Type.h"

void setNextTimeout(void);
void timerTick();

#define TIMER_INTERRUPT 2
#define SOFTWARE_TRAP 1
#define UNKNOWN_DEVICE 0

typedef struct TimeSpec {
    u64 second;
    long microSecond;    
} TimeSpec;

typedef struct CpuTimes {
    long user;
    long kernel;
    long deadChildrenUser;
    long deadChildrenKernel;
} CpuTimes;


#endif