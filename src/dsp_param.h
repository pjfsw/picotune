#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"
#include <stdatomic.h>

#define NUMBER_OF_VOICES 4
#define VOICE_DOWN_MIX_BITS 0

typedef enum {
    WAV_SAW = 0,
    WAV_SQUARE = 1,
    WAV_TRIANGLE = 2,
    WAV_NOISE = 3,
    WAV_COUNT = 4 // Number of wave forms
} Waveform;

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint32_t phase_add;
    uint16_t volume;
    uint32_t noise_phase_inc;
    uint8_t table_weight;
    uint8_t pwm;
    Waveform waveform;
    bool highpass;
} DspChannel;

typedef struct {
    DspChannel channel[NUMBER_OF_VOICES];
     _Atomic uint32_t control_id;
} DspParam;

#endif