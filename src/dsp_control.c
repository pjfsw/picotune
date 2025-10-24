#include <math.h>
#include <string.h>
#include "dsp_control.h"
#include "samplerate.h"
#include "wavetable.h"

extern DspParam dsp_param;

static DspControl singleton;

static void reset_registers() {
    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        dsp_param.channel[i].waveform = WAV_SAW;
        dsp_param.channel[i].phase_add = 0;
        dsp_param.channel[i].volume = 0;
        dsp_param.channel[i].pwm = 0;
        dsp_param.channel[i].highpass = false;
        get_wavetable_for_frequency(440, &dsp_param.channel[i]);
    }
    dsp_param.control_id = 0;
}

static void init_volume_maps(DspControl *dspc) {
    float peak_level[4] = {-4,-6,-3,-4};
    for (int wav = 0; wav < 4; wav++) {
        for (int i = 0; i < VOLUME_STEPS; i++) {
            float dbfs = peak_level[wav]-(VOLUME_STEPS-1-i);
            float level = 65535.0 * powf(10, dbfs/20.0);
            dspc->volmap[wav][i] = (uint16_t)level;
        }
    }
}

DspControl *dspc_singleton() {
    return &singleton;
}

void dspc_init(DspControl *dspc) {
    memset(dspc, 0, sizeof(DspControl));
    reset_registers();
    init_volume_maps(dspc);
}

// Integer: phase_inc = round(f_update / fs * 2^32)
static inline uint32_t phase_inc_from_rate(uint32_t f_update) {
    if (f_update >= SAMPLE_RATE) {
        return 0xFFFFFFFFu; // extremely fast; will wrap a lot
    }
    uint64_t num = ((uint64_t)f_update << 32); // f_update * 2^32
    // + fs/2 for rounding
    return (uint32_t)((num + ((uint32_t)SAMPLE_RATE>>1)) / (uint32_t)SAMPLE_RATE);
}

static uint32_t get_noise_phase_inc(uint32_t f_update) {
        // Clamp to Nyquist to avoid useless alias brightness
    if (f_update > (uint32_t)SAMPLE_RATE/2) {
        f_update = (uint32_t)SAMPLE_RATE/2;
    }
    if (f_update < 1) {
        f_update = 1;
    }
    return phase_inc_from_rate(f_update);
}

static inline void refresh_wavetable(DspControl *dspc, int voice) {
    get_wavetable_for_frequency(dspc->registers.voices[voice].frequency8>>3, &dsp_param.channel[voice]);         
}

static inline void transform_voice_frequency(DspControl *dspc, int voice) {
    uint32_t frequency8 = dspc->registers.voices[voice].frequency8;
    dsp_param.channel[voice].phase_add = (uint32_t)((((uint64_t)frequency8) << 29) / SAMPLE_RATE);
    refresh_wavetable(dspc, voice);
    dsp_param.channel[voice].noise_phase_inc = get_noise_phase_inc(frequency8>>1);
}

void dspc_set_frequency(DspControl *dspc, int voice, uint16_t frequency8) {
    dspc->registers.voices[voice].frequency8 = frequency8;
    transform_voice_frequency(dspc, voice);
}

static void transform_voice_control(DspControl *dspc, int voice) {
    uint8_t volume = dspc->registers.voices[voice].control & 0x3f;
    uint8_t waveform = dspc->registers.voices[voice].control >> 6;
    dsp_param.channel[voice].volume = dspc->volmap[waveform][volume];
    dsp_param.channel[voice].waveform = waveform;
    refresh_wavetable(dspc, voice);
}

void dspc_set_control(DspControl *dspc, int voice, uint8_t control_value) {
    dspc->registers.voices[voice].control = control_value;
    transform_voice_control(dspc, voice);
}

static void transform_voice_pwm(DspControl *dspc, int voice) {
    dsp_param.channel[voice].pwm = dspc->registers.voices[voice].pwm;
}

void dspc_set_pwm(DspControl *dspc, int voice, uint8_t pwm) {
    dspc->registers.voices[voice].pwm = pwm;
    transform_voice_pwm(dspc, voice);
}

void dspc_latch() {
    atomic_fetch_add_explicit(&dsp_param.control_id, 1, memory_order_release);
}


