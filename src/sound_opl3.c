#include "config.h"
#ifdef ENABLE_SOUND_DRIVER_OPL3
#include "sound.h"
#include "system.h"
#include "irq.h"
#include "math.h"

/*
 * https://github.com/DhrBaksteen/ArduinoOPL2/blob/master/indepth.md
 * https://www.fit.vutbr.cz/~arnost/opl/opl3.html
 * http://map.grauw.nl/resources/sound/yamaha_ymf262.pdf
 * http://shipbrook.net/jeff/sb.html
 */

#ifndef ADLIB_ADDR_BASE
#define ADLIB_ADDR_BASE 0x0388
#endif

#define ADLIB_ADDR_IDX (ADLIB_ADDR_BASE + 0)
#define ADLIB_ADDR_DATA (ADLIB_ADDR_BASE + 1)
#define ADLIB_ADDR_IDX_H (ADLIB_ADDR_BASE + 2)
#define ADLIB_ADDR_DATA_H (ADLIB_ADDR_BASE + 3)

#define ADLIB_NOTE_ON (0b00100000)

static const u8 OPERATOR_1_MAP[] = {
    0, 1, 2, 6, 7, 8
};
static const u8 OPERATOR_2_MAP[] = {
    3, 4, 5, 9, 10, 11
};

struct Instrument_Op {
    u8 ad;
    u8 sr;
    u8 kslol;

    u8 fbsyn;
    u8 waveform;

    u8 tre_vib_sus_mul;
};

struct Instrument {
    u8 fbsyn;
    struct Instrument_Op op1;
    struct Instrument_Op op2;
};

struct Instrument electric_piano = {
    .fbsyn = 0b00110010,
    .op2 = {
        .ad = 0xF1,
        .sr = 0xD8,
        .kslol = 0b00000000,
        .waveform = 0,
        .tre_vib_sus_mul = 0b00100001,
    },
    .op1 = {
        .ad = 0xF1,
        .sr = 0xC8,
        .kslol = 0b01010111,
        .waveform = 0,
        .tre_vib_sus_mul = 0b00000001,
    },
};

struct Instrument drum = {
    .fbsyn = 0b00111110,
    .op2 = {
        .ad = 0xF6,
        .sr = 0x16,
        .kslol = 0b00000000,
        .waveform = 0,
        .tre_vib_sus_mul = 0b00000100,
    },
    .op1 = {
        .ad = 0xF0,
        .sr = 0x00,
        .kslol = 0b00000000,
        .waveform = 2,
        .tre_vib_sus_mul = 0b00001100,
    },
};

static u8 notes2[NUM_NOTES];

static void opl_write_register_high(u8 index, u8 value) {
    outportb(ADLIB_ADDR_IDX_H, index);
    outportb(ADLIB_ADDR_DATA_H, value);
}

static void opl_write_register(u8 index, u8 value) {
    outportb(ADLIB_ADDR_IDX, index);
    outportb(ADLIB_ADDR_DATA, value);
}

static void sound_adjust(u8 index, struct Instrument inst) {
    u8 baseindex = OPERATOR_1_MAP[index];
    opl_write_register(baseindex + 0x20, inst.op1.tre_vib_sus_mul);
    opl_write_register(baseindex + 0x40, inst.op1.kslol);
    opl_write_register(baseindex + 0x60, inst.op1.ad);
    opl_write_register(baseindex + 0x80, inst.op1.sr);
    opl_write_register(baseindex + 0xA0, 0);
    opl_write_register(baseindex + 0xB0, 0);
    opl_write_register(index + 0xC0, inst.fbsyn);
    opl_write_register(baseindex + 0xE0, inst.op1.waveform);

    baseindex = OPERATOR_2_MAP[index];
    opl_write_register(baseindex + 0x20, inst.op2.tre_vib_sus_mul);
    opl_write_register(baseindex + 0x40, inst.op2.kslol);
    opl_write_register(baseindex + 0x60, inst.op2.ad);
    opl_write_register(baseindex + 0x80, inst.op2.sr);
    opl_write_register(baseindex + 0xA0, 0);
    opl_write_register(baseindex + 0xB0, 0);
    opl_write_register(baseindex + 0xE0, inst.op2.waveform);
}

void sound_note(u8 index, u8 octave, note_t note) {
    u8 baseindex = OPERATOR_1_MAP[index];
    opl_write_register(baseindex + 0xB0, notes2[index] & ~ADLIB_NOTE_ON);
    if (note == NOTE_NONE) {
        return;
    }
    opl_write_register(baseindex + 0xA0, note & 0xFF);
    u8 note2 = ((note >> 8) & 0b11) | octave << 2 | ADLIB_NOTE_ON;
    notes2[index] = note2;
    opl_write_register(baseindex + 0xB0, note2);
}

void sound_master(u8 v) {
}

void sound_wave(u8 index, u8 wave) {
    switch (wave) {
        case WAVE_BASS:
        case WAVE_HARMONY:
        case WAVE_MELODY:
            sound_adjust(index, electric_piano);
            break;
        case WAVE_SNARE:
            sound_adjust(index, drum);
            break;
    }
}

void sound_init() {
    opl_write_register_high(0x05, 1);
    opl_write_register(0x01, 0b00100000);
}

#endif
