#include "wavetable.h"
#include <math.h>
#include <stdio.h>

#define WAVETABLE_COUNT 8
#define WAVETABLE_NOTES_PER_TABLE_SHIFT 4
#define WAVETABLE_NOTES_PER_TABLE (1<<WAVETABLE_NOTES_PER_TABLE_SHIFT)

uint32_t wavetables[WAVETABLE_COUNT][WAVETABLE_SIZE];

static int16_t get_wavetable_amp(int index, float base_frequency) {
    float x = (float)index / WAVETABLE_SIZE;
    float s = 0;
    float harm = 1.0;
    while (harm * base_frequency < 20000.0) {    
        s = s + sinf(harm * 2.0f * M_PI * x) / harm;  // -1.0 → +1.0
        harm += 2.0;
    }
    //s = 4 * s / M_PI;
    if (s < -1) {
        s = -1;
    } else if (s > 1 ) {
        s = 1;
    }
    return (int16_t)(s * 32767);
}

static void create_wavetable(uint32_t* wavetable, float base_frequency) {
    for (int i = 0; i < WAVETABLE_SIZE; i++) {
        int n = (i+1) & (WAVETABLE_SIZE-1);
        uint16_t u1 = (uint16_t)get_wavetable_amp(i, base_frequency);
        uint16_t u2 = (uint16_t)get_wavetable_amp(n, base_frequency);
        wavetable[i] = u1 | (u2<<16);
    }
}

void wavetable_init() {
    int note = WAVETABLE_NOTES_PER_TABLE;
    for (int table = 1; table < WAVETABLE_COUNT; table++) {
        float base_frequency = 440.0 * powf(2.0f, (note-69)/12.0f);
        printf("Initialize table %d, base frequency %f\n", table, base_frequency);
        if (base_frequency < 20.0) {
            base_frequency = 20.0;
        }
        create_wavetable(wavetables[table], base_frequency);
        note += WAVETABLE_NOTES_PER_TABLE;
    }
}

static inline float freq_to_midi(float freq_hz) {
    return 69.0f + 12.0f * log2f(freq_hz / 440.0f);
}

uint32_t *get_wavetable_for_frequency(float f) {
    int midi_note = (int)lroundf(freq_to_midi(f));
    int t = 1;
    if ((midi_note > 1) && (midi_note < WAVETABLE_COUNT * WAVETABLE_NOTES_PER_TABLE)) { 
        t = midi_note >> WAVETABLE_NOTES_PER_TABLE_SHIFT;
    }
    printf("Freq %f, table #%d\n", f, t);
    return wavetables[t];
}