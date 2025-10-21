#ifndef VOICE_H
#define VOICE_H

#include "pico/stdlib.h"

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint16_t volume;
    uint32_t phase_add;
    uint32_t control_id;
    uint32_t phase_diff;
    uint8_t table_weight;
    uint8_t amp_shift;
    bool highpass;
    bool use_phase_diff;
    bool use_noise;    
} VoiceParam;

// 23-bit LFSR + variable-rate clock, integer-only
typedef struct {
    uint32_t lfsr;       // use lower 23 bits, never 0
    uint32_t phase;      // 32-bit phase accumulator
    uint32_t phase_inc;  // maps desired update rate to 32-bit increment
} Noise;


// Highpass stuff
typedef struct {
    int32_t x1;   // previous input
    int32_t y1;   // previous output
} HPState;

// Lowpass stuff
typedef struct {
    int32_t lp, bp;
} SVF;

typedef struct {
    uint8_t target;
    int32_t current;
    int32_t step;
    int32_t remaining;
} Ramp;

typedef struct {
    VoiceParam voice_param;    
    Ramp ramp;
    HPState hp_state;
    Noise noise;
    SVF svf;
    uint32_t phase;
} Voice;

#endif 