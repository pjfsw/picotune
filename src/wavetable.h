#ifndef _WAVETABLE_H
#define _WAVETABLE_H

#include "wavetable_data.h"
#include "pico/stdlib.h"
#include "dsp_param.h"

void wavetable_init();

void get_wavetable_for_frequency(float f, DspChannel *dsp_channel);

#endif