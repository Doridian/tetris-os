#include "timer.h"
#include "isr.h"
#include "irq.h"

#define PIT_A 0x40
#define PIT_B 0x41
#define PIT_C 0x42
#define PIT_CONTROL 0x43

#define PIT_MASK 0xFF
#define PIT_SET 0x36

#define PIT_HZ 1193181
#define DIV_OF_FREQ(_f) (PIT_HZ / (_f))
#define FREQ_OF_DIV(_d) (PIT_HZ / (_d))
#define REAL_FREQ_OF_FREQ(_f) (FREQ_OF_DIV(DIV_OF_FREQ((_f))))

static struct {
    u32 frequency;
    u32 divisor;
    u32 ticks;
} state;

static void timer_set(int hz) {
    outportb(PIT_CONTROL, PIT_SET);

    u16 d = (u16) (PIT_HZ / hz);
    outportb(PIT_A, d & PIT_MASK);
    outportb(PIT_A, (d >> 8) & PIT_MASK);
}

u32 timer_get() {
    return state.ticks;
}

static void timer_handler(struct Registers *regs) {
    state.ticks++;
}

void timer_init() {
    const u32 freq = REAL_FREQ_OF_FREQ(TIMER_TPS);
    state.frequency = freq;
    state.divisor = DIV_OF_FREQ(freq);
    state.ticks = 0;
    //timer_set(state.divisor);
    timer_set(TIMER_TPS);
    irq_install(0, timer_handler);
}
