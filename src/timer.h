#ifndef TIMER_H
#define TIMER_H

#include "util.h"

#define PIT_HZ 1193181.666
#define DIV_OF_FREQ(_f) (PIT_HZ / (_f))
#define FREQ_OF_DIV(_d) (PIT_HZ / (_d))
#define REAL_FREQ_OF_FREQ(_f) (FREQ_OF_DIV(DIV_OF_FREQ((_f))))

#define TIMER_TPS_TARGET 600
#define TIMER_TPS ((i16)REAL_FREQ_OF_FREQ(TIMER_TPS_TARGET))

u32 timer_get();
void timer_init();

#endif
