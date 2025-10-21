#ifndef _SYNTH_H
#define _SYNTH_H

#include "pico/stdlib.h"
#include "voice.h"

void synth_init(Voice *voice);

int32_t synth_next_sample(Voice *voice);

#endif