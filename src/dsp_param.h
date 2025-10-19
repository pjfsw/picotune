#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"

typedef struct {
    uint32_t *wavetable;
    uint16_t volume;
    uint32_t phase_add;
} DspParam;

#endif