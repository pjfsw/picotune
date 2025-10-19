#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include "pico/stdlib.h"
#include "dsp_param.h"

#define WAVETABLE_BITS 11
#define WAVETABLE_SIZE (1<<WAVETABLE_BITS)
#define WAVETABLE_SHIFT (32-WAVETABLE_BITS)

void wavetable_init();

void get_wavetable_for_frequency(float f, volatile DspParam *dsp_param);

#endif