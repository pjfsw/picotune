#include "synth.h"
#include "samplerate.h"

#define SYNTH_MAX_AMP 2000*1000*1000

static int32_t amp = SYNTH_MAX_AMP;
static uint16_t t = 0;

extern volatile uint8_t g_volume;

int32_t synth_next_sample() {
    t++;
    if (t >= SAMPLE_RATE/441) {
        amp = -amp;
        t = 0;
    }

    int64_t tmp = (int64_t)amp * (int64_t)g_volume;
    return tmp >> 8;
}
