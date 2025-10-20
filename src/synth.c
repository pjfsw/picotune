#include "synth.h"
#include "samplerate.h"
#include "dsp_param.h"
#include "wavetable.h"
#include "voice.h"

#define FRAC_BITS 8
#define FRAC_SHIFT (WAVETABLE_SHIFT-FRAC_BITS)
#define FRAC_MASK ((1<<FRAC_BITS)-1)


void synth_init() {
    //ramp.current = 0;
    //ramp.remaining = 0;
}

static int64_t get_oscillator(VoiceParam *voice_param, uint32_t phase) {
    uint32_t index = phase >> WAVETABLE_SHIFT;  // 0..2047
    uint32_t remainder = (phase >> FRAC_SHIFT) & FRAC_MASK; // 0..255
    uint8_t table_weight = voice_param->table_weight;
    uint32_t va = voice_param->wavetable[index];    
    int16_t va1 = (int16_t)((uint16_t)(va & 0xFFFF));
    int16_t va2 = (int16_t)((uint16_t)(va >> 16));
    int32_t a1 = (int32_t)va1 * (int32_t)(FRAC_MASK-remainder) * (int32_t)(255-table_weight);
    int32_t a2 = (int32_t)va2 * (int32_t)remainder * (int32_t)(255-table_weight);
        
    uint32_t vb = voice_param->wavetable2[index];
    int16_t vb1 = (int16_t)((uint16_t)(vb & 0xFFFF));
    int16_t vb2 = (int16_t)((uint16_t)(vb >> 16));
    int32_t b1 = (int32_t)vb1 * (int32_t)(FRAC_MASK-remainder) * (int32_t)(table_weight);
    int32_t b2 = (int32_t)vb2 * (int32_t)remainder * (int32_t)(table_weight);

    return (int64_t)(a1+a2+b1+b2);
}


int32_t synth_next_sample(Voice *voice) {
    voice->phase += voice->voice_param.phase_add;
    int64_t osc1 = get_oscillator(&voice->voice_param, voice->phase);
    int64_t osc2 = get_oscillator(&voice->voice_param, voice->phase + voice->voice_param.phase_diff);
    int64_t out = (osc2-osc1)>>1;
    
    uint16_t new_volume = voice->voice_param.volume;
    if (voice->ramp.target != new_volume) {
        voice->ramp.target = new_volume;
        voice->ramp.remaining = 48;
        int32_t target = new_volume;
        voice->ramp.step = (target - voice->ramp.current)/voice->ramp.remaining;    
    }
    if (voice->ramp.remaining > 0) {
        voice->ramp.current += voice->ramp.step;
        voice->ramp.remaining--;
    } else {
        voice->ramp.current = (int32_t)voice->ramp.target;
    }
   
    int64_t tmp = ((int64_t)out * (int64_t)voice->ramp.current);
    tmp = tmp >> 16; // compensate for remainder (8 bits) and table weight (8 bits)
    return tmp;
}
