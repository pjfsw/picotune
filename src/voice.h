#ifndef VOICE_H
#define VOICE_H

#include "pico/stdlib.h"

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint16_t volume;
    uint32_t phase_add;
    uint8_t table_weight;
    uint32_t control_id;
} VoiceParam;

typedef struct {
    uint8_t target;
    int32_t current;
    int32_t step;
    int32_t remaining;
} Ramp;

typedef struct {
    VoiceParam voice_param;    
    Ramp ramp;
    uint32_t phase;
} Voice;

#endif 