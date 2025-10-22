#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"

#define NUMBER_OF_VOICES 4

typedef enum {
    WAV_SAW = 0,
    WAV_SQUARE = 1,
    WAV_TRIANGLE = 2,
    WAV_NOISE = 3
} Waveform;

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint32_t phase_add;
    uint32_t control_id;
    uint16_t volume;
    Waveform waveform;
    uint32_t noise_phase_inc;
    // Polyblep
    int32_t  dt_q31;         // ≈ phase_inc converted to Q1.31
    int32_t  inv_dt_q31;     // ≈ 1/dt in Q1.31 (reciprocal)    
    // other
    uint8_t table_weight;
    uint8_t phase_diff;
    bool highpass;
} DspParam;

#endif