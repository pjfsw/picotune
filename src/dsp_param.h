#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"

#define WAVETABLE_BITS 11
#define WAVETABLE_SIZE (1<<WAVETABLE_BITS)
#define WAVETABLE_SHIFT (32-WAVETABLE_BITS)

typedef struct {
    uint16_t volume;
    uint32_t phase_add;
} DspParam;

#endif