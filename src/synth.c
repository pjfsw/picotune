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
    uint8_t table_weight = dsp_param.table_weight;
    uint32_t va = dsp_param.wavetable[index];    
    int16_t va1 = (int16_t)((uint16_t)(va & 0xFFFF));
    int16_t va2 = (int16_t)((uint16_t)(va >> 16));
    int32_t a1 = (int32_t)va1 * (int32_t)(FRAC_MASK-remainder) * (int32_t)(255-table_weight);
    int32_t a2 = (int32_t)va2 * (int32_t)remainder * (int32_t)(255-table_weight);
        
    uint32_t vb = dsp_param.wavetable2[index];
    int16_t vb1 = (int16_t)((uint16_t)(vb & 0xFFFF));
    int16_t vb2 = (int16_t)((uint16_t)(vb >> 16));
    int32_t b1 = (int32_t)vb1 * (int32_t)(FRAC_MASK-remainder) * (int32_t)(table_weight);
    int32_t b2 = (int32_t)vb2 * (int32_t)remainder * (int32_t)(table_weight);
    
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
   
    int64_t tmp = ((int64_t)(a1+a2+b1+b2) * (int64_t)ramp.current);
    tmp = tmp >> 16; // compensate for remainder (8 bits) and table weight (8 bits)
    return tmp;
}
