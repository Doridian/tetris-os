#ifndef MUSIC_H
#define MUSIC_H

#include "util.h"
#include "timer.h"

#define TRACK_BPM 150
#define TRACK_BPMS (TRACK_BPM * 1000 / 60)
#define TICKS_PER_BEAT 16

#define TIMER_TICKS_PER_MUSIC_TICK ((TIMER_TPS * 1000) / TRACK_BPMS / TICKS_PER_BEAT)

void music_tick(u32 ticks);
void music_init();

#endif
