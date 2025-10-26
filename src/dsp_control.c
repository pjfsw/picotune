#include <math.h>
#include <string.h>
#include "dsp_control.h"
#include "samplerate.h"
#include "wavetable.h"

extern DspSettings dsp_settings;

static DspData dsp_data[2];
static DspData *current_dsp_data = &dsp_data[0];
static int current_dsp_ptr = 0;

static DspControl singleton;

static void reset_registers(DspChannel *dsp_channel) {
    memset(dsp_channel, 0, sizeof(DspChannel));
    get_wavetable_for_frequency(440, dsp_channel);
}

static void init_volume_maps(DspControl *dspc) {
//    float peak_level[4] = {-4,-6,-3,-4};
    float peak_level[4] = {-3,-5,-2,-3};
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

static inline void set_current_dsp_data() {
    current_dsp_data = &dsp_data[current_dsp_ptr]; 
}

void dspc_init(DspControl *dspc) {
    memset(dspc, 0, sizeof(DspControl));
    for (int i = 0 ; i < NUMBER_OF_VOICES; i++) {
        reset_registers(&dsp_data[0].channels[i]);
        reset_registers(&dsp_data[1].channels[i]);
    }
    dsp_settings.control_id = 0;
    current_dsp_ptr = 0;
    set_current_dsp_data();
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
    get_wavetable_for_frequency(dspc->registers.voices[voice].frequency8>>3, &current_dsp_data->channels[voice]);         
}

static inline void transform_voice_frequency(DspControl *dspc, int voice) {
    uint32_t frequency8 = dspc->registers.voices[voice].frequency8;
    current_dsp_data->channels[voice].phase_add = (uint32_t)((((uint64_t)frequency8) << 29) / SAMPLE_RATE);
    refresh_wavetable(dspc, voice);
    current_dsp_data->channels[voice].noise_phase_inc = get_noise_phase_inc(frequency8>>1);
}

void dspc_set_frequency(DspControl *dspc, int voice, uint16_t frequency8) {
    dspc->registers.voices[voice].frequency8 = frequency8;
    transform_voice_frequency(dspc, voice);
}

static void transform_voice_control(DspControl *dspc, int voice) {
    uint8_t volume = dspc->registers.voices[voice].control & 0x3f;
    uint8_t waveform = dspc->registers.voices[voice].control >> 6;
    current_dsp_data->channels[voice].volume = dspc->volmap[waveform][volume];
    current_dsp_data->channels[voice].waveform = waveform;
    refresh_wavetable(dspc, voice);
}

void dspc_set_control(DspControl *dspc, int voice, uint8_t control_value) {
    dspc->registers.voices[voice].control = control_value;
    transform_voice_control(dspc, voice);
}

static void transform_voice_envelope(DspControl *dspc, int voice) {
    uint16_t envelope = dspc->registers.voices[voice].envelope;
    current_dsp_data->channels[voice].adsr.release = dspc->adsr.release_map[(envelope >> 0) & 15];
    current_dsp_data->channels[voice].adsr.sustain = (((envelope >> 4) & 15) << 11) + 34815;
    current_dsp_data->channels[voice].adsr.decay = dspc->adsr.decay_map[(envelope >> 8) & 15];
    current_dsp_data->channels[voice].adsr.attack = dspc->adsr.attack_map[(envelope >> 12) & 3];
    current_dsp_data->channels[voice].gate = (envelope >> ENV_GATE_BIT) & 1;
    current_dsp_data->channels[voice].filter_enable = (envelope >> ENV_FILTER_BIT) & 1;
}

void dspc_set_envelope(DspControl *dspc, int voice, uint16_t env) {
    dspc->registers.voices[voice].envelope = env;
    transform_voice_envelope(dspc, voice);
}
static void transform_voice_pwm(DspControl *dspc, int voice) {
    current_dsp_data->channels[voice].pwm = dspc->registers.voices[voice].pwm;
}

void dspc_set_pwm(DspControl *dspc, int voice, uint8_t pwm) {
    dspc->registers.voices[voice].pwm = pwm;
    transform_voice_pwm(dspc, voice);
}

#define LPF_FC_MIN 0x005D5F79 // (~20Hz)
#define LPF_FC_MAX 0x80000000 // 7350 Hz 
static void transform_lp_fc(DspControl *dspc) {
    current_dsp_data->filter.lp_fc = LPF_FC_MIN + (int32_t)(((int64_t)(LPF_FC_MAX - LPF_FC_MIN) * dspc->registers.lp_fc) >> 16); // Q31 
}

void dspc_set_filter_lp_fc(DspControl *dspc, uint16_t fc) {
    dspc->registers.lp_fc = fc;
    transform_lp_fc(dspc);
}

//#define LPF_Q_MIN 0x46666666 //Q dampening=0.55
//#define LPF_Q_MIN 0x2CCCCCCC //Q damping 0.35
//#define LPF_Q_MIN 0x26666666 //Q damping 0.3
#define LPF_Q_MIN 0x20000000 // Q damping 0.25 <-- Souds sick!
#define LPF_Q_MAX 0x7999999a //Q damping 0.95
static void transform_lp_q(DspControl *dspc) {
    current_dsp_data->filter.lp_q = LPF_Q_MIN + (int32_t)(((int64_t)(LPF_Q_MAX - LPF_Q_MIN) * dspc->registers.lp_q) >> 8);  // Q31 
}

void dspc_set_filter_lp_q(DspControl *dspc, uint8_t q) {
    dspc->registers.lp_q = q;
    transform_lp_q(dspc);
}

void dspc_latch() {
    dsp_settings.dsp_data = current_dsp_data;
    atomic_fetch_add_explicit(&dsp_settings.control_id, 1, memory_order_release);
    memcpy(&dsp_data[1 - current_dsp_ptr], &dsp_data[current_dsp_ptr], sizeof(DspData));
    current_dsp_ptr = 1 - current_dsp_ptr;
    set_current_dsp_data();
}


