#ifndef _TIMER_H_
#define _TIMER_H_

#define INTERVAL (390000000 / 200)

void setNextTimeout(void);
void timerTick();

#define TIMER_INTERRUPT 2
#define SOFTWARE_TRAP 1
#define UNKNOWN_DEVICE 0

#endif