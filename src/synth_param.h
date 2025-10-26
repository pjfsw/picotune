#ifndef SYNTH_PARAM_H
#define SYNTH_PARAM_H

#include "pico/stdlib.h"

typedef enum {
    WAV_SAW = 0,
    WAV_SQUARE = 1,
    WAV_TRIANGLE = 2,
    WAV_NOISE = 3,
    WAV_COUNT = 4 // Number of wave forms
} Waveform;

typedef struct {
    uint16_t attack;
    uint32_t decay;
    uint16_t sustain;
    uint32_t release;
} Adsr;

typedef struct {
    int32_t hp_fc; // Cut off frequency, ~20-7350 Hz
    int32_t lp_fc; // Cut off frequency, ~20-7350 Hz
    int32_t lp_q;  // Q damping factor, 0 = most resonance, 255=least resonance
} FilterParam;

#endif