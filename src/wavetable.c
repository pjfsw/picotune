#include "wavetable.h"
#include <math.h>
#include <stdio.h>

#define WAVETABLE_NOTES_PER_TABLE_SHIFT 4
#define WAVETABLE_NOTES_PER_TABLE (1<<WAVETABLE_NOTES_PER_TABLE_SHIFT)

static inline float freq_to_midi(float freq_hz) {
    return 69.0f + 12.0f * log2f(freq_hz / 440.0f);
}

static uint8_t table_weight[16]= {17,34,51,68,85,102,119,136,153,170,187,204,221,238,255,255};

void get_wavetable_for_frequency(float f, DspChannel *dsp_channel) {
    int midi_note = (int)lroundf(freq_to_midi(f));
    int t = 1;
    if ((midi_note > 1) && (midi_note < WAVETABLE_COUNT * WAVETABLE_NOTES_PER_TABLE)) { 
        t = midi_note >> WAVETABLE_NOTES_PER_TABLE_SHIFT;
    }
    if (dsp_channel->waveform == WAV_TRIANGLE) {
        dsp_channel->wavetable = wavetables_triangle[t];
        dsp_channel->wavetable2 = wavetables_triangle[t+1];  
    } else {
        dsp_channel->wavetable = wavetables_saw[t];
        dsp_channel->wavetable2 = wavetables_saw[t+1];  
    }
    dsp_channel->table_weight = table_weight[midi_note & 15];
}