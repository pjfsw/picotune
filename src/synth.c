#include <math.h>
#include "synth.h"
#include "samplerate.h"
#include "dsp_param.h"

#define SYNTH_MAX_AMP 2000*1000*1000

static uint32_t phase = 0;

extern volatile DspParam dsp_param;

static uint32_t wavetable[WAVETABLE_SIZE];

//#define WAVETABLE_REMAINDER (WAVETABLE_SIZE-1)

#define FRAC_BITS 8
#define FRAC_SHIFT (WAVETABLE_SHIFT-FRAC_BITS)
#define FRAC_MASK ((1<<FRAC_BITS)-1)

typedef struct {
    uint8_t target;
    int32_t current;
    int32_t step;
    int32_t remaining;
} Ramp;

static Ramp ramp;

static int16_t get_wavetable_amp(int index) {
    float x = (float)index / WAVETABLE_SIZE;
    float s = 0;
    for (int i = 1; i <= 13; i+=2) {
        float harm = (float)i;
        s = s + sinf(harm * 2.0f * M_PI * x) / harm;  // -1.0 → +1.0
    }
    //s = 4 * s / M_PI;
    if (s < -1) {
        s = -1;
    } else if (s > 1 ) {
        s = 1;
    }
    return (int16_t)(s * 32767);
}

void synth_init() {
    for (int i = 0; i < WAVETABLE_SIZE; i++) {
        int n = (i+1) & (WAVETABLE_SIZE-1);
        uint16_t u1 = (uint16_t)get_wavetable_amp(i);
        uint16_t u2 = (uint16_t)get_wavetable_amp(n);
        wavetable[i] = u1 | (u2<<16);
    }
    ramp.current = 0;
    ramp.remaining = 0;
}

int32_t synth_next_sample() {
    phase += dsp_param.phase_add;
    uint32_t index = phase >> WAVETABLE_SHIFT;  // 0..2047
    uint32_t remainder = (phase >> FRAC_SHIFT) & FRAC_MASK; // 0..255
    uint32_t v = wavetable[index];    
    int16_t v1 = (int16_t)((uint16_t)(v & 0xFFFF));
    int16_t v2 = (int16_t)((uint16_t)(v >> 16));
    int32_t a1 = (int32_t)v1 * (int32_t)(FRAC_MASK-remainder);
    int32_t a2 = (int32_t)v2 * (int32_t)remainder;

    uint16_t new_volume = dsp_param.volume;
    if (ramp.target != new_volume) {
        ramp.target = new_volume;
        ramp.remaining = 48;
        int32_t target = new_volume;
        ramp.step = (target - ramp.current)/ramp.remaining;    
    }
    if (ramp.remaining > 0) {
        ramp.current += ramp.step;
        ramp.remaining--;
    } else {
        ramp.current = (int32_t)ramp.target;
    }
   
    int64_t tmp = ((int64_t)(a1+a2) * (int64_t)ramp.current);
    tmp = tmp >> 8;
    return tmp;
}
