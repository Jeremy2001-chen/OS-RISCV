#include <Driver.h>
#include <Timer.h>
#include <Process.h>
#include <Riscv.h>

static u32 ticks;

void setNextTimeout() {
    SBI_CALL_1(SBI_SET_TIMER, r_time() + INTERVAL);
}

void timerTick() {
    ticks++;
    setNextTimeout();
}