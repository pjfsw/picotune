#ifndef VOICE_H
#define VOICE_H

#include "pico/stdlib.h"

#define ENVELOPE_SCALE_BITS 11 // Amplitude of the envelope dB-to-V table
#define ENVELOPE_SCALE_SHIFT (16-ENVELOPE_SCALE_BITS)
#define ENVELOPE_SCALE (1<<ENVELOPE_SCALE_BITS)

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint16_t volume;
    uint32_t phase_add;
    uint32_t pwm;
    uint16_t attack;
    uint32_t decay;
    uint16_t sustain;
    uint32_t release;
    uint8_t table_weight;
    bool highpass;
    bool use_pwm;
    bool use_noise;    
    bool gate;   
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
    int32_t target;
    int32_t current;
    int32_t step;
    int32_t remaining;
} Ramp;

typedef enum {
    ENV_OFF = 0,
    ENV_ATTACK = 1,
    ENV_DECAY = 2,
    ENV_SUSTAIN = 3,
    ENV_RELEASE = 4,
} EnvState;

typedef struct {
    VoiceParam voice_param;    
    Ramp ramp;
    HPState hp_state;    
    Noise noise;
    SVF svf;
    uint32_t phase;
    EnvState env_state;
    uint16_t *env_table;
    bool last_gate;
} Voice;

#endif 