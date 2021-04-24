#include "config.h"
#ifdef ENABLE_SOUND_DRIVER_SB16

#include "sound.h"
#include "sound_sb16.h"
#include "system.h"
#include "irq.h"
#include "music.h"
#include "waveforms.h"
#include "string_util.h"

static const int OCTAVE[8] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
static const int NOTES[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};

#define MIXER_IRQ       0x5
#define MIXER_IRQ_DATA  0x2

// SB16 ports
#define SB_BASE        0x220
#define DSP_MIXER       (SB_BASE + 0x4)
#define DSP_MIXER_DATA  (SB_BASE + 0x5)
#define DSP_RESET       (SB_BASE + 0x6)
#define DSP_READ        (SB_BASE + 0xA)
#define DSP_WRITE       (SB_BASE + 0xC)
#define DSP_READ_STATUS (SB_BASE + 0xE)
#define DSP_ACK_8       DSP_READ_STATUS
#define DSP_ACK_16      (SB_BASE + 0xF)

// TODO: ???
#define DSP_PROG_16     0xB0
#define DSP_PROG_8      0xC0
#define DSP_AUTO_INIT   0x06
#define DSP_PLAY        0x00
#define DSP_RECORD      0x08
#define DSP_MONO        0x00
#define DSP_STEREO      0x20
#define DSP_UNSIGNED    0x00
#define DSP_SIGNED      0x10

#define DMA_CHANNEL_16  5
#define DMA_FLIP_FLOP   0xD8
#define DMA_BASE_ADDR   0xC4
#define DMA_COUNT       0xC6

// commands for DSP_WRITE
#define DSP_SET_TIME    0x40
#define DSP_SET_RATE    0x41
#define DSP_ON          0xD1
#define DSP_OFF         0xD3
#define DSP_OFF_8       0xD0
#define DSP_ON_8        0xD4
#define DSP_OFF_16      0xD5
#define DSP_ON_16       0xD6
#define DSP_VERSION     0xE1

// commands for DSP_MIXER
#define DSP_VOLUME  0x22
#define DSP_IRQ     0x80

#define SAMPLE_RATE     22050
#define BUFFER_MS       50

#define BUFFER_SIZE ((size_t) (SAMPLE_RATE * BUFFER_MS / 1000))

static __attribute__ ((aligned (4096))) i16 buffer[BUFFER_SIZE];
static bool buffer_flip = false;

static int sample_pos[NUM_NOTES];
static u8 volume_master;
static u8 volumes[NUM_NOTES];
static u8 notes[NUM_NOTES];
static u8 waves[NUM_NOTES];

void sound_note(u8 index, u8 octave, note_t note) {
    notes[index] = (octave << 4) | note;
}

void sound_volume(u8 index, u8 v) {
    volumes[index] = v;
}

void sound_master(u8 v) {
    volume_master = v;
}

void sound_wave(u8 index, u8 wave, u8 volume) {
    waves[index] = wave;
    volumes[index] = volume;
}

static void fill(i16 *buf, size_t len) {
    static const unsigned int samples_per_tick = SAMPLE_RATE * 1000 / TRACK_BPMS / TICKS_PER_BEAT;
    static unsigned int sample_counter = samples_per_tick;
            
    for (size_t i = 0; i < len; i++) {
        int sample = 0;

        if (sample_counter-- == 0) {
            sample_counter = samples_per_tick;
            music_tick(1);
        }

        for (size_t j = 0; j < NUM_NOTES; j++) {
            int s = 0;
            u8 octave = (notes[j] >> 4) & 0xF,
               note = notes[j] & 0xF;

            if (note == NOTE_NONE) {
                sample_pos[j] = 0;
                continue;
            }

            // using 24.8 fixed point
            int note_freq = OCTAVE[octave] * NOTES[note];
            int note_phase = note_freq * 0x100 / SAMPLE_RATE;
            sample_pos[j] = (sample_pos[j] + note_phase) % wave_len_fix;
                    
            switch (waves[j]) {
                case WAVE_SIN: {
                    s = (int)((signed char)(wave_sine[sample_pos[j] / 0x100])) * 128;
                    break;
                }
                case WAVE_SQUARE: {
                    s = (int)((signed char)(wave_square[sample_pos[j] / 0x100])) * 128;
                    break;
                }
                case WAVE_TRIANGLE: {
                    s = (int)((signed char)(wave_triangle[sample_pos[j] / 0x100])) * 128;
                    break;
                }
                case WAVE_NOISE: {
                    s = ((signed int)rand()) / (32768 * 8);
                    break;
                }
            }

            sample += s * volumes[j] / 256;
        }

        buf[i] = (i16) (sample * volume_master / 256);
    }
}

static void dsp_write(u8 b) {
    while (inportb(DSP_WRITE) & 0x80);
    outportb(DSP_WRITE, b);
}

static u8 dsp_read(u8 b) {
    while (inportb(DSP_READ_STATUS) & 0x80);
    return inportb(DSP_READ);
}

static bool dsp_detect_timeout(u8* b) {
    for (size_t i = 0; i < 1000000; i++) {
        if (inportb(DSP_READ_STATUS) & 0x80) {
            *b = inportb(DSP_READ);
            return true;
        }
    }

    return false;
}

static void reset() {
    char buf0[128], buf1[128];
    u8 status = 0, reason = 0;
    
    outportb(DSP_RESET, 1);

    // TODO: maybe not necessary
    // ~3 microseconds?
    for (size_t i = 0; i < 1000000; i++);

    outportb(DSP_RESET, 0);

    if (!dsp_detect_timeout(&status) || status != 0xAA) {
        reason = 1;
        goto fail;
    }

    outportb(DSP_WRITE, DSP_VERSION);
    u8 major = inportb(DSP_READ),
       minor = inportb(DSP_READ);

    if (major < 4) {
        reason = 3;
        status = (major << 4) | minor;
        goto fail;
    }

    return;
fail:
    strlcpy(buf0, "FAILED TO RESET SB16: ", 128);
    
    itoa_e(reason, buf1, 128);
    strlcat(buf0, buf1, 128);
    
    strlcat(buf0, "/", 128);

    itoa_e(status, buf1, 128);
    strlcat(buf0, buf1, 128);
    
    panic(buf0);
}

static void set_sample_rate(u16 hz) {
    dsp_write(DSP_SET_RATE);
    dsp_write((u8) ((hz >> 8) & 0xFF));
    dsp_write((u8) (hz & 0xFF));
}

static void transfer(void *buf, u32 len) {
    // disable DMA channel
    outportb(DSP_ON_8, 4 + (DMA_CHANNEL_16 % 4));

    // clear byte-poiner flip-flop
    outportb(DMA_FLIP_FLOP, 0);

    // write DMA mode for transfer
    outportb(DSP_ON_16, (DMA_CHANNEL_16 % 4) + 0x58);

    // write buffer offset (div 2 for 16-bit)
    u16 offset = (((uintptr_t) buf) / 2) & 0xFFFF;
    outportb(DMA_BASE_ADDR, (u8) ((offset >> 0) & 0xFF));
    outportb(DMA_BASE_ADDR, (u8) ((offset >> 8) & 0xFF));

    // write transfer length
    outportb(DMA_COUNT, (u8) (((len - 1) >> 0) & 0xFF));
    outportb(DMA_COUNT, (u8) (((len - 1) >> 8) & 0xFF));

    // write buffer page
    outportb(0x8B, ((uintptr_t) buf) >> 16);

    // enable DMA channel
    outportb(DSP_ON_8, DMA_CHANNEL_16 % 4);
}

static void sb16_irq_handler(struct Registers *regs) {
    buffer_flip = !buffer_flip;

    fill(
        &buffer[buffer_flip ? 0 : (BUFFER_SIZE / 2)],
        (BUFFER_SIZE / 2)
    );

    //inportb(DSP_READ_STATUS);
    inportb(DSP_ACK_16);
    outportb(0x20, 0x20);
    outportb(0xA0, 0x20);
}

static void configure() {
    irq_install(MIXER_IRQ, sb16_irq_handler);
    outportb(DSP_MIXER, DSP_IRQ);
    outportb(DSP_MIXER_DATA, MIXER_IRQ_DATA);

    u8 v = MIXER_IRQ;
    if (v != MIXER_IRQ) {
        char buf0[128], buf1[128];
        itoa_e(v, buf0, 128);
        strlcpy(buf1, "SB16 HAS INCORRECT IRQ: ", 128);
        strlcat(buf1, buf0, 128);
        panic(buf1);
    }
}

void sound_init() {
    reset();
    configure();

    transfer(buffer, BUFFER_SIZE);
    set_sample_rate(SAMPLE_RATE);

    u16 sample_count = (BUFFER_SIZE / 2) - 1;
    dsp_write(DSP_PLAY | DSP_PROG_16 | DSP_AUTO_INIT);
    dsp_write(DSP_SIGNED | DSP_MONO);
    dsp_write((u8) ((sample_count >> 0) & 0xFF));
    dsp_write((u8) ((sample_count >> 8) & 0xFF));

    dsp_write(DSP_ON);
    dsp_write(DSP_ON_16);

    memset(&notes, NOTE_NONE, sizeof(notes));
    memset(&waves, WAVE_SIN, sizeof(waves));
    memcpy(buffer, wave_sine, 256);
}

#endif
