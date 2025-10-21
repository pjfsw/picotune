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
    uint8_t table_weight;
    uint8_t phase_diff;
    Waveform waveform;
    bool highpass;
    uint32_t noise_phase_inc;
} DspParam;

#endif