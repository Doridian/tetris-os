#ifndef SOUND_OPL3_H
#define SOUND_OPL3_H

typedef u16 note_t;

#define OCTAVE_1 0
#define OCTAVE_2 1
#define OCTAVE_3 2
#define OCTAVE_4 3
#define OCTAVE_5 4
#define OCTAVE_6 5
#define OCTAVE_7 6

#define NOTE_C      342
#define NOTE_CS     363
#define NOTE_DF     NOTE_CS
#define NOTE_D      385
#define NOTE_DS     408
#define NOTE_EF     NOTE_DS
#define NOTE_E      432
#define NOTE_F      458
#define NOTE_FS     485
#define NOTE_GF     NOTE_FS
#define NOTE_G      514
#define NOTE_GS     544
#define NOTE_AF     NOTE_GS
#define NOTE_A      577
#define NOTE_AS     611
#define NOTE_BF     NOTE_AS
#define NOTE_B      647

#define NOTE_NONE   0

#define WAVE_MELODY  0
#define WAVE_SNARE   1
#define WAVE_BASS    2
#define WAVE_HARMONY 3

void sound_wave(u8 index, u8 wave);

#endif
