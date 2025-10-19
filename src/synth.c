#include "synth.h"
#include "samplerate.h"
#include "dsp_param.h"
#include "wavetable.h"

#define FRAC_BITS 8
#define FRAC_SHIFT (WAVETABLE_SHIFT-FRAC_BITS)
#define FRAC_MASK ((1<<FRAC_BITS)-1)


static uint32_t phase = 0;

extern volatile DspParam dsp_param;

typedef struct {
    uint8_t target;
    int32_t current;
    int32_t step;
    int32_t remaining;
} Ramp;

static Ramp ramp;


void synth_init() {
    ramp.current = 0;
    ramp.remaining = 0;
}

int32_t synth_next_sample() {
    phase += dsp_param.phase_add;
    uint32_t index = phase >> WAVETABLE_SHIFT;  // 0..2047
    uint32_t remainder = (phase >> FRAC_SHIFT) & FRAC_MASK; // 0..255
    uint32_t v = dsp_param.wavetable[index];    
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
