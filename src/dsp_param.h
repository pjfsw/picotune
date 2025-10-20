#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint16_t volume;
    uint32_t phase_add;
    uint8_t table_weight;
} DspParam;

#endif