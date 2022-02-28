#ifndef _TIMER_H_
#define _TIMER_H_

#define INTERVAL 200000

void setNextTimeout(void);
void timerTick();

#define TIMER_INTERRUPT 2
#define SOFTWARE_TRAP 1
#define UNKNOWN_DEVICE 0

#endif