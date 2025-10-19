#ifndef _SYNTH_H
#define _SYNTH_H

#include "pico/stdlib.h"

void synth_init();

int32_t synth_next_sample();

#endif