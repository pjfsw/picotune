#include "synth.h"
#include "samplerate.h"
#include "dsp_param.h"
#include "wavetable.h"
#include "voice.h"

#define FRAC_BITS 8
#define FRAC_SHIFT (WAVETABLE_SHIFT-FRAC_BITS)
#define FRAC_MASK ((1<<FRAC_BITS)-1)

#define HP_120HZ 0x7DD3BEF0
#define HP_200HZ 0x7C7E5B51
#define HP_400HZ 0x78C454BD
#define HP_800HZ 0x7210D7DB

void synth_init(Voice *voice) {
    voice->noise.lfsr = (uint32_t)time_us_32() & 0x7FFFFF;
     if (!voice->noise.lfsr) {
         voice->noise.lfsr = 1;
     }
    voice->noise.phase = 0;
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

    return (int64_t)a1+(int64_t)a2+(int64_t)b1+(int64_t)b2;
}


// a in Q31, e.g. for 80 Hz at 44.1k:  a_q31 = 0x7E8CA0EA (≈0.9886666)
static inline int32_t highpass(HPState *st, int32_t x, int32_t a_q31) {
    // y = (x - x1) + a*y1
    int64_t acc = (int64_t)a_q31 * (int64_t)st->y1;  // Q31 * Q31 = Q62
    int32_t ay1 = (int32_t)(acc >> 31);              // back to Q31
    int64_t y = (int64_t)x - (int64_t)st->x1 + (int64_t)ay1;

    // Optional: saturate to 32-bit
    if (y > INT32_MAX) {
        y = INT32_MAX;
    }
    if (y < INT32_MIN) {
        y = INT32_MIN;
    }

    st->x1 = x;
    st->y1 = (int32_t)y;
    return (int32_t)y;
}

// Q31 multiply: (a*b)>>31 with 64-bit intermediate
static inline int32_t q31_mul(int32_t a, int32_t b) {
    return (int32_t)(( (int64_t)a * (int64_t)b ) >> 31);
}

static inline int32_t svf_lowpass(SVF *s, int32_t x, int32_t f_q31, int32_t q_q31) {
    // hp = x - lp - q*bp
    int32_t qbp = q31_mul(q_q31, s->bp);
    int32_t hp  = x - s->lp - qbp;

    // bp += f*hp
    s->bp += q31_mul(f_q31, hp);

    // lp += f*bp
    s->lp += q31_mul(f_q31, s->bp);

    // Optional gentle clamp to avoid windup in extreme settings
    if (s->lp > INT32_MAX) {
        s->lp = INT32_MAX;
    }
    if (s->lp < INT32_MIN) {
         s->lp = INT32_MIN;
    }
    if (s->bp > INT32_MAX) {
         s->bp = INT32_MAX;
    }
    if (s->bp < INT32_MIN) {
         s->bp = INT32_MIN;
    }

    return s->lp; // low-pass output
}

static inline uint32_t lfsr23_step(uint32_t s){
    // taps: bit22 ^ bit17 -> LSB; 23-bit state kept in 0..22
    uint32_t fb = ((s >> 22) ^ (s >> 17)) & 1u;
    s = ((s << 1) & 0x7FFFFFu) | fb;
    if (!s) {
        s = 1;
    }
    return s;
}

static inline int32_t noise23_next(Noise *n){
    uint32_t p = n->phase + n->phase_inc;
    // Single-wrap detect; for extreme rates you can loop:
    if (p < n->phase) {
        n->lfsr = lfsr23_step(n->lfsr);
    }
    n->phase = p;

    int32_t v = (int32_t)((n->lfsr >> 7) & 0xFFFF);
    v = (v - 32768) << 16;
    return v;
}

int32_t synth_next_sample(Voice *voice) {
    voice->phase += voice->voice_param.phase_add;
    int64_t osc1 = voice->voice_param.use_noise ? noise23_next(&voice->noise) : get_oscillator(&voice->voice_param, voice->phase);
    int64_t out = osc1;
    if (voice->voice_param.use_pwm) {
        int64_t osc2 = get_oscillator(&voice->voice_param, voice->phase + voice->voice_param.pwm);
        out = (osc2-osc1);
    }
    out = out >> 1;

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
    int32_t tmp32 = tmp >> 16; // compensate for remainder (8 bits) and table weight (8 bits)    
    if (voice->voice_param.highpass) {
        tmp32 = highpass(&voice->hp_state, tmp32, HP_200HZ);
    }
    // if voice->voice_param.lowpass) {
        // int32_t f_q31 = (int32_t)roundf( (2.0f * sinf((float)M_PI * Fc / Fs)) * 2147483648.0f );
        // int32_t q_q31 = (int32_t)roundf(q_damp * 2147483648.0f);

        //tmp32 = svf_lowpass(&voice->svf, tmp32, 0x03A5B2BC, 0x5999999A);
    //}
    return tmp32;
}
