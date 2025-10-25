#include <math.h>
#include <string.h>
#include "dsp_control.h"
#include "samplerate.h"
#include "wavetable.h"

extern DspParam dsp_param;

static DspChannel dsp_channels[2][NUMBER_OF_VOICES];
static DspChannel *current_dsp_channels = dsp_channels[0];
static int current_dsp_ptr = 0;

static DspControl singleton;

static void reset_registers(DspChannel *dsp_channel) {
    dsp_channel->waveform = WAV_SAW;
    dsp_channel->phase_add = 0;
    dsp_channel->volume = 0;
    dsp_channel->pwm = 0;
    dsp_channel->highpass = false;
    get_wavetable_for_frequency(440, dsp_channel);
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

static inline uint32_t ms_to_sample_buffers(float ms) {
    uint32_t buffers = (uint32_t)roundf(ms * SAMPLE_RATE / 1000.0 / (float)BUF_LEN);
    return buffers*BUF_LEN;
}

static inline void set_decay_release(DspControl *dspc, int index, float ms) {
    uint32_t v = ms_to_sample_buffers(ms);
    dspc->adsr.decay_map[index] = v;
    dspc->adsr.release_map[index] = v;
}

static void init_envelope_maps(DspControl *dspc) {
    dspc->adsr.attack_map[0] = ms_to_sample_buffers(1.0);
    dspc->adsr.attack_map[1] = ms_to_sample_buffers(8.0);
    dspc->adsr.attack_map[2] = ms_to_sample_buffers(32.0);
    dspc->adsr.attack_map[3] = ms_to_sample_buffers(256.0);
    set_decay_release(dspc, 0, 4.0);
    set_decay_release(dspc, 1, 16.0);
    set_decay_release(dspc, 2, 32.0);
    set_decay_release(dspc, 3, 48.0);
    set_decay_release(dspc, 4, 72.0);
    set_decay_release(dspc, 5, 100.0);
    set_decay_release(dspc, 6, 140.0);
    set_decay_release(dspc, 7, 180.0);
    set_decay_release(dspc, 8, 240.0);
    set_decay_release(dspc, 9, 350.0);
    set_decay_release(dspc, 10, 500.0);
    set_decay_release(dspc, 11, 750.0);
    set_decay_release(dspc, 12, 1000.0);
    set_decay_release(dspc, 13, 2000.0);
    set_decay_release(dspc, 14, 3000.0);
    set_decay_release(dspc, 15, 4000.0); 
}


DspControl *dspc_singleton() {
    return &singleton;
}

static inline void set_current_dsp_channels() {
    current_dsp_channels = dsp_channels[current_dsp_ptr]; 
}

void dspc_init(DspControl *dspc) {
    memset(dspc, 0, sizeof(DspControl));
    for (int i = 0 ; i < NUMBER_OF_VOICES; i++) {
        reset_registers(&dsp_channels[0][i]);
        reset_registers(&dsp_channels[1][i]);
    }
    dsp_param.control_id = 0;
    current_dsp_ptr = 0;
    set_current_dsp_channels();
    init_volume_maps(dspc);
    init_envelope_maps(dspc);
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
    get_wavetable_for_frequency(dspc->registers.voices[voice].frequency8>>3, &current_dsp_channels[voice]);         
}

static inline void transform_voice_frequency(DspControl *dspc, int voice) {
    uint32_t frequency8 = dspc->registers.voices[voice].frequency8;
    current_dsp_channels[voice].phase_add = (uint32_t)((((uint64_t)frequency8) << 29) / SAMPLE_RATE);
    refresh_wavetable(dspc, voice);
    current_dsp_channels[voice].noise_phase_inc = get_noise_phase_inc(frequency8>>1);
}

void dspc_set_frequency(DspControl *dspc, int voice, uint16_t frequency8) {
    dspc->registers.voices[voice].frequency8 = frequency8;
    transform_voice_frequency(dspc, voice);
}

static void transform_voice_control(DspControl *dspc, int voice) {
    uint8_t volume = dspc->registers.voices[voice].control & 0x3f;
    uint8_t waveform = dspc->registers.voices[voice].control >> 6;
    current_dsp_channels[voice].volume = dspc->volmap[waveform][volume];
    current_dsp_channels[voice].waveform = waveform;
    refresh_wavetable(dspc, voice);
}

void dspc_set_control(DspControl *dspc, int voice, uint8_t control_value) {
    dspc->registers.voices[voice].control = control_value;
    transform_voice_control(dspc, voice);
}

static void transform_voice_envelope(DspControl *dspc, int voice) {
    uint16_t envelope = dspc->registers.voices[voice].envelope;
    current_dsp_channels[voice].release = dspc->adsr.release_map[(envelope >> 0) & 15];
    current_dsp_channels[voice].sustain = ((envelope >> 4) & 15) << 12;
    current_dsp_channels[voice].decay = dspc->adsr.decay_map[(envelope >> 8) & 15];
    current_dsp_channels[voice].attack = dspc->adsr.attack_map[(envelope >> 12) & 3];
    current_dsp_channels[voice].gate = (envelope >> ENV_GATE_BIT) & 1;
    current_dsp_channels[voice].highpass = (envelope >> ENV_HPF_BIT) & 1;
}

void dspc_set_envelope(DspControl *dspc, int voice, uint16_t env) {
    dspc->registers.voices[voice].envelope = env;
    transform_voice_envelope(dspc, voice);
}
static void transform_voice_pwm(DspControl *dspc, int voice) {
    current_dsp_channels[voice].pwm = dspc->registers.voices[voice].pwm;
}

void dspc_set_pwm(DspControl *dspc, int voice, uint8_t pwm) {
    dspc->registers.voices[voice].pwm = pwm;
    transform_voice_pwm(dspc, voice);
}

void dspc_latch() {
    dsp_param.channels = current_dsp_channels;
    atomic_fetch_add_explicit(&dsp_param.control_id, 1, memory_order_release);
    memcpy(dsp_channels[1 - current_dsp_ptr], dsp_channels[current_dsp_ptr], NUMBER_OF_VOICES * sizeof(DspChannel));
    current_dsp_ptr = 1 - current_dsp_ptr;
    set_current_dsp_channels();
}


