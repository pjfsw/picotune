#ifndef DSP_CONTROL_H
#define DSP_CONTROL_H

#include "pico/stdlib.h"
#include "dsp_param.h"

#define VOLUME_STEPS 64
#define ENV_GATE_BIT 15
#define ENV_HPF_BIT 14

typedef struct {
    uint16_t frequency8;    // Frequency * 8 (Alt: Bit 8..14 midinote, bit 0..7 = midinote fractional part)
    uint16_t envelope;      // Bit 15 = gate, bit 14 = highpass
                            // bit 12-13 = Attack, Bit 8-11 = Decay, Bit 4-7 = Sustain, bit 0-3 = Release
    uint8_t control;        // Bit 0..5 volume, bit 6-7 waveform
    uint8_t pwm;            // PWM byte 0..255
} VoiceRegisters;

typedef struct {
    VoiceRegisters voices[NUMBER_OF_VOICES];
} SynthRegisters; // 18 bytes*/

typedef struct {
    uint16_t attack_map[4];
    uint32_t decay_map[16];
    uint32_t release_map[16];
} Adsr;

typedef struct {
    uint16_t volmap[WAV_COUNT][VOLUME_STEPS];
    SynthRegisters registers;
    Adsr adsr;
} DspControl;

DspControl *dspc_singleton();

void dspc_init(DspControl *);

void dspc_set_frequency(DspControl *dspc, int voice, uint16_t frequency8);

void dspc_set_control(DspControl *dspc, int voice, uint8_t control_value);

void dspc_set_envelope(DspControl *dspc, int voice, uint16_t env);

void dspc_set_pwm(DspControl *dspc, int voice, uint8_t pwm);

void dspc_latch();

#endif
