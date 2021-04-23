#ifndef SOUND_H
#define SOUND_H

#include "config.h"
#include "util.h"

#ifdef ENABLE_SOUND_DRIVER_OPL3
#include "sound_opl3.h"
#endif
#ifdef ENABLE_SOUND_DRIVER_SB16
#include "sound_sb16.h"
#endif

#define NUM_NOTES 8

#define NUM_OCTAVES 7
#define OCTAVE_SIZE 12

void sound_init();
void sound_master(u8 v);
void sound_note(u8 index, u8 octave, note_t note);

#endif
