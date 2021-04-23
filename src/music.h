#ifndef MUSIC_H
#define MUSIC_H

#include "util.h"

#define TRACK_BPM 150
#define TRACK_BPMS (TRACK_BPM * 1000 / 60)
#define TICKS_PER_BEAT 16

void music_tick();
void music_init();

#endif
