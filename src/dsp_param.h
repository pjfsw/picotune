#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"

#define NUMBER_OF_VOICES 4

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint32_t phase_add;
    uint32_t control_id;
    uint16_t volume;
    uint8_t table_weight;
    uint8_t phase_diff;
    bool highpass;
} DspParam;

#endif