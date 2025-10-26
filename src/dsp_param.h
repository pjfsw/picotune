#ifndef _DSP_PARAM
#define _DSP_PARAM

#include "pico/stdlib.h"
#include <stdatomic.h>
#include "synth_param.h"

#define BUF_LEN 16

#define NUMBER_OF_VOICES 4
#define VOICE_DOWN_MIX_BITS 0

#define ENVELOPE_BITS 10       // Number of entries in the envelope dB-to-V table
#define ENVELOPE_STEPS (1<<ENVELOPE_BITS)
#define ENVELOPE_SHIFT (16-ENVELOPE_BITS)
#define ENVELOPE_FRACTIONAL_BITS 8

typedef struct {
    const uint32_t *wavetable;
    const uint32_t *wavetable2;
    uint32_t phase_add;
    uint16_t volume;
    uint32_t noise_phase_inc;
    Waveform waveform;
    Adsr adsr;
    uint8_t table_weight;
    uint8_t pwm;
    bool filter_enable;
    bool gate;   
} DspChannel;

typedef struct {
    FilterParam filter;
    DspChannel channels[NUMBER_OF_VOICES];
    uint8_t mode;  
} DspData;

typedef struct {    
    DspData *dsp_data;
     _Atomic uint32_t control_id;
} DspSettings;

#endif