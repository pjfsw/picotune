#ifndef DSP_CONTROL_H
#define DSP_CONTROL_H

#include "pico/stdlib.h"
#include "dsp_param.h"

#define VOLUME_STEPS 64
#define ENV_GATE_BIT 15
#define ENV_FILTER_BIT 14

/*
    Register map:

    Byte | Chan | Synth mode              | Sample mode
    00   | 0    |         Frequency*8 low byte   
    01   | 0    |         Frequency*8 high byte
    02   | 0    | S(4-7), R(0-3)          | -
    03   | 0    | Gate(7), A(4-6), D(0-3) | Gate(7)
    04   | 0    | Wave(6-7), Volume(0-5)  | Volume(0-5)
    05   | 0    | PWM                     |
    06   | 1    | ...                     |
    0c   | 2    | ...                     |
    12   | 3    | ...                     |
    18   | All  |                  Lowpass Fc low byte
    19   | All  |                  Lowpass Fc high byte
    1a   | All  |                  Highpass Fc low byte
    1b   | All  |                  Highpass Fc high byte
    1c   | All  |                     Lowpass Q  
    1d   | All  | Filter mode 
*/

typedef struct {
    uint16_t frequency8;    // Frequency * 8 (Alt: Bit 8..14 midinote, bit 0..7 = midinote fractional part)
    uint16_t envelope;      // Bit 15 = gate, bit 14 = filter on
                            // bit 12-13 = Attack, Bit 8-11 = Decay, Bit 4-7 = Sustain, bit 0-3 = Release
    uint8_t control;        // Bit 0..5 volume, bit 6-7 waveform
    uint8_t pwm;            // PWM byte 0..255
} VoiceRegisters;

typedef struct {
    VoiceRegisters voices[NUMBER_OF_VOICES];
    uint16_t lp_fc;
    uint16_t hp_fc;
    uint8_t lp_q;
    uint8_t mode;           // Bit 4..7 1=sample mode, 0=voice mode,  Bit 0..3 = filter type (1=HP,0=LP)
} SynthRegisters; // 30 bytes*/

typedef struct {
    uint16_t attack_map[4];
    uint32_t decay_map[16];
    uint32_t release_map[16];
} AdsrMap;

typedef struct {
    uint16_t volmap[WAV_COUNT][VOLUME_STEPS];
    SynthRegisters registers;
    AdsrMap adsr;
} DspControl;

DspControl *dspc_singleton();

void dspc_init(DspControl *);

void dspc_set_frequency(DspControl *dspc, int voice, uint16_t frequency8);

void dspc_set_control(DspControl *dspc, int voice, uint8_t control_value);

void dspc_set_envelope(DspControl *dspc, int voice, uint16_t env);

void dspc_set_pwm(DspControl *dspc, int voice, uint8_t pwm);

void dspc_set_filter_lp_fc(DspControl *dspc, uint16_t fc);

void dspc_set_filter_lp_q(DspControl *dspc, uint8_t q);

void dspc_set_filter_hp_fc(DspControl *dspc, uint16_t fc);

void dspc_set_mode(DspControl *dspc, uint8_t control_value);

void dspc_latch();

void dspc_transform(DspControl *dspc);

#endif
