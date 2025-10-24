#ifndef DSP_CONTROL_H
#define DSP_CONTROL_H

#include "pico/stdlib.h"
#include "dsp_param.h"

#define VOLUME_STEPS 64

typedef struct {
    uint16_t frequency8;    // Frequency * 8
    uint8_t control;        // Bit 0..5 volume, bit 6-7 waveform select (00=Saw, 01=Pulse, 10=Triangle, 11=Noise)
    uint8_t pwm;            // PWM byte 0..255
} VoiceRegisters;

typedef struct {
    VoiceRegisters voices[NUMBER_OF_VOICES];
    uint16_t global_control; // Bit 0..5 global volume,
                             // bit 6..15 reserved (filter setting per channel etc)
} SynthRegisters; // 18 bytes*/

typedef struct {
    uint16_t volmap[WAV_COUNT][VOLUME_STEPS];
    SynthRegisters registers;
} DspControl;

DspControl *dspc_singleton();

void dspc_init(DspControl *);

void dspc_set_frequency(DspControl *, int voice, uint16_t frequency8);

void dspc_set_control(DspControl *, int voice, uint8_t control_value);

void dspc_set_pwm(DspControl *, int voice, uint8_t pwm);

void dspc_latch();

#endif
