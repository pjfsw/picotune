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
    int32_t hp_fc; 
    int32_t lp_fc; // Cut off frequency, 0x005D5F79 = 20 Hz, 0x8A1FFF5D = 8000 Hz
    int32_t lp_q;  // Q, resonance, 0x46666666 = 0.55, 0x7999999A = 0.95
} FilterParam;

#endif