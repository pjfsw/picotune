#ifndef WAVETABLE_DATA_H
#define WAVETABLE_DATA_H

#include <stdint.h>

#define WAVETABLE_COUNT 8
#define WAVETABLE_BITS 11
#define WAVETABLE_SIZE (1<<WAVETABLE_BITS)
#define WAVETABLE_SHIFT (32-WAVETABLE_BITS)

extern const uint32_t wavetables_saw[WAVETABLE_COUNT][WAVETABLE_SIZE];
extern const uint32_t wavetables_triangle[WAVETABLE_COUNT][WAVETABLE_SIZE];

#endif