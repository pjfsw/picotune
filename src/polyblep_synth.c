#include "polyblep_synth.h"

static inline int32_t q31_mul(int32_t a, int32_t b) {
    return (int32_t)(((int64_t)a * (int64_t)b) >> 31);
}

static inline int32_t q31_clamp(int64_t x) {
    if (x > INT32_MAX) {
        return INT32_MAX;
    }
    if (x < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)x;
}

// Convert Q0.32 to Q1.31 by shifting right 1
static inline int32_t q31_from_q32(uint32_t q32) {
    return (int32_t)(q32 >> 1);
}

// t: phase in [0,1) Q1.31
// dt: step size Q1.31
// inv_dt: reciprocal of dt Q1.31
static inline int32_t polyblep_q31(int32_t t, int32_t dt, int32_t inv_dt) {
    // region 1: 0 <= t < dt
    if (t >= 0 && t < dt) {
        // x = t/dt - 1
        int32_t x = q31_mul(t, inv_dt) - INT32_C(0x7FFFFFFF);
        // 0.5 * x*x
        return (int32_t)(( (int64_t)q31_mul(x, x) ) >> 1);
    }
    // region 2: 1-dt < t < 1
    int32_t one_minus_t = INT32_C(0x7FFFFFFF) - t; // 1 - t
    if (one_minus_t >= 0 && one_minus_t < dt) {
        // x = (t-1)/dt + 1  == 1 - (1-t)/dt
        int32_t x = INT32_C(0x7FFFFFFF) - q31_mul(one_minus_t, inv_dt);
        return (int32_t)(( (int64_t)q31_mul(x, x) ) >> 1);
    }
    return 0;
}

// frac(a - b) for Q1.31 (wrap into [0,1))
static inline int32_t frac_sub_q31(int32_t a, int32_t b) {
    int32_t r = a - b;
    // wrap: Q1.31 range is [-1, +1). We want [0,1) -> add 1 if negative.
    if (r < 0) {
        r += INT32_C(0x7FFFFFFF);
    }
    return r;
}

// Saw: y = 2t - 1  then subtract PolyBLEP at wrap
static inline int32_t osc_saw_q31(Voice *voice) {
    int32_t t = q31_from_q32(voice->phase);                 // Q1.31 in [0,1)
    int32_t y = (int32_t)(((int64_t)t << 1) - INT32_C(0x7FFFFFFF)); // 2t-1
    int32_t blep = polyblep_q31(t, voice->voice_param.poly_blep.dt_q31, voice->voice_param.poly_blep.inv_dt_q31);
    return y - blep; // subtract at the single discontinuity
}

// PWM square: naive square +/-1 plus PolyBLEP at both edges
static inline int32_t osc_pwm_q31(Voice* voice, int32_t duty) {
    PolyBlep *poly_blep = &voice->voice_param.poly_blep;
    int32_t t = q31_from_q32(voice->phase);             // [0,1)

    int32_t y = (t < duty) ? INT32_C(0x7FFFFFFF) : -INT32_C(0x7FFFFFFF);

    // Rising edge at t=0 (wrap)
    int32_t blep_rise = polyblep_q31(t, poly_blep->dt_q31, poly_blep->inv_dt_q31);
    // Falling edge at t=duty
    int32_t td = frac_sub_q31(t, duty);
    int32_t blep_fall = polyblep_q31(td, poly_blep->dt_q31, poly_blep->inv_dt_q31);

    // Square needs +blep at rising, -blep at falling
    // Scale by 2 because the discontinuity is 2.0 between -1 and +1
    int64_t acc = (int64_t)y + ((int64_t)blep_rise - (int64_t)blep_fall) * 2;
    return q31_clamp(acc);
}


// Triangle via leaky integration of BLEP square
// leak ~ 1e-4 (Q1.31 ≈ 214748) prevents DC drift
static inline int32_t osc_triangle_q31(Voice *voice, int32_t leak_q31) {
    // Use PWM with duty=0.5
    int32_t s = osc_pwm_q31(voice, INT32_C(0x40000000));

    // leaky integrator: z = z + s - leak*z
    int32_t leak_term = q31_mul(leak_q31, voice->tri_z);
    int64_t z = (int64_t)voice->tri_z + (int64_t)s - (int64_t)leak_term;
    voice->tri_z = q31_clamp(z);

    // rough scaling to keep triangle amplitude near +/-1
    return voice->tri_z;
}

int32_t polyblep_synth_next_sample(Voice *voice) {
    voice->phase += voice->voice_param.phase_add;

    int32_t out = osc_pwm_q31(voice, voice->voice_param.phase_add);

    uint16_t new_volume = voice->voice_param.volume;
    if (voice->ramp.target != new_volume) {
        voice->ramp.target = new_volume;
        voice->ramp.remaining = 48;
        int32_t target = new_volume;
        voice->ramp.step = (target - voice->ramp.current) / voice->ramp.remaining;
    }
    if (voice->ramp.remaining > 0) {
        voice->ramp.current += voice->ramp.step;
        voice->ramp.remaining--;
    } else {
        voice->ramp.current = (int32_t)voice->ramp.target;
    }

    int64_t tmp = ((int64_t)out * (int64_t)voice->ramp.current);
    int32_t tmp32 = tmp >> 16;  // compensate for remainder (8 bits) and table weight (8 bits)
    /*if (voice->voice_param.highpass) {
        tmp32 = highpass(&voice->hp_state, tmp32, HP_200HZ);
    }*/
    // if voice->voice_param.lowpass) {
    // int32_t f_q31 = (int32_t)roundf( (2.0f * sinf((float)M_PI * Fc / Fs)) * 2147483648.0f );
    // int32_t q_q31 = (int32_t)roundf(q_damp * 2147483648.0f);

    //tmp32 = svf_lowpass(&voice->svf, tmp32, 0x03A5B2BC, 0x5999999A);
    //}
    return tmp32;
}