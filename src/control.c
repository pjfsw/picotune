#include <math.h>
#include "control.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "samplerate.h"
#include "dsp_param.h"
#include "wavetable.h"
#include "dsp_control.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

typedef struct {
    int8_t notes[NUMBER_OF_VOICES][32];
} Song;

typedef struct {
    uint16_t freq8table[128];
    Song song;
} Control;

static Control control;
static DspControl *dspc; 

void control_init() {
    dspc = dspc_singleton();
    dspc_init(dspc);
}

static void init_freq8_table(Control *control) {
    for (int i = 0; i < 128; i++) {
        float f = 440.0 * powf(2.0f, (i-69)/12.0f);
        control->freq8table[i] = (uint16_t)lroundf(f * 8.0);
    }
}

#define LED_PIN 25
#define DEBUG_PIN 22

uint8_t step = 0;
uint8_t songpos = 0;
int8_t vol[NUMBER_OF_VOICES]; 
uint8_t pwm[NUMBER_OF_VOICES];

typedef struct {
    int8_t vol;
    uint8_t pwm;
    uint8_t wf;
} Instr;

Instr instr[NUMBER_OF_VOICES];

uint16_t adsr[NUMBER_OF_VOICES] = {0x43bd, 0x399e, 0x0475,0x02b6};

#define FILTER_DROP_START 0xc000
#define FILTER_DROP_END 0x0c00
#define FILTER_DROP_SPEED 0x1000
#define FILTER_Q 0

int32_t filter_sweep_value = 0;

static inline void sequencer_callback() {
    dspc_latch();

    uint8_t track_pos = songpos & 0x1f;
    Song *song = &control.song;
    if (step == 5) {
        for (int i = 0; i < NUMBER_OF_VOICES; i++) {            
            int8_t note = song->notes[i][track_pos];
            if (note > 0) {
                // Clear gate bit just before next note
                dspc_set_envelope(dspc, i, adsr[i]);
            }            
        }
    }
    if (step == 6) {
        for (int i = 0; i < NUMBER_OF_VOICES; i++) {            
            int8_t note = song->notes[i][track_pos];
            if (note > 0) {
                uint16_t freq8 = control.freq8table[note];
                dspc_set_frequency(dspc, i, freq8);
                pwm[i] = 63;
                vol[i] = instr[i].vol;
                dspc_set_envelope(dspc, i, adsr[i] | (1<<ENV_GATE_BIT));
                if (i == 0) {
                    filter_sweep_value = FILTER_DROP_START;
                }
            } else if (note < 0) {
                dspc_set_envelope(dspc, i, adsr[i]);
            }
        }
        songpos++;
        step = 0;
    } else {
        step++;
    }
    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        dspc_set_control(dspc, i, instr[i].wf | vol[i]);
        dspc_set_pwm(dspc, i, pwm[i]);
        pwm[i]+=instr[i].pwm;
    }
    dspc_set_filter_lp_fc(dspc, filter_sweep_value);
    dspc_set_filter_lp_q(dspc, FILTER_Q);
    filter_sweep_value -= FILTER_DROP_SPEED;
    if (filter_sweep_value < FILTER_DROP_END) {
        filter_sweep_value = FILTER_DROP_END;
    }

}

static void init_song(Song *song) {
    memset(song, 0, sizeof(Song));
    int8_t bass_notes[32] = {38,0,0,-1,0,0,38,0,38,50,0,38,50,0,38,0, 34,0,-1,0,46,0,34,0,34,46,0,34,46,0,48,0};
    int8_t mid_notes[32] = {62,0,0,0,0,0,-1,0, 64,0,0,0,0,0,-1,0,65,0,0,0,0,0,67,0,64,0,0,-1,60,0,0,-1};
    int8_t mid2_notes[32] = {57,-1,62,-1,74,-1,74,-1,57,-1,62,-1,74,-1,74,-1,57,-1,62,-1,74,-1,74,-1,57,-1,62,-1,76,77,72,74};
    int8_t rythm_notes[32] = {0,0,0,0,100,-1,0,0,0,0,0,0,100,-1,0,0,0,0,0,0,100,-1,0,0,0,0,0,0,100,-1,100,-1};
    memcpy(&song->notes[0], bass_notes, 32);
    memcpy(&song->notes[1], mid_notes, 32);
    memcpy(&song->notes[2], mid2_notes, 32);
    memcpy(&song->notes[3], rythm_notes, 32);
    instr[0].vol = 63;
    instr[0].pwm = 2;
    instr[0].wf = 0x40;
    instr[1].vol = 58;
    instr[1].pwm = 3;
    instr[1].wf = 0x40;
    instr[2].vol = 58;
    instr[2].wf = 0x00;
    instr[3].vol = 63;
    instr[3].wf = 0xc0;
}

static inline void pulse() {
    gpio_xor_mask(1u << DEBUG_PIN);
    sequencer_callback();
}

void __isr pwm_wrap_isr() {
    pwm_clear_irq(0);

    pulse();
}

void start_60hz_pwm_irq(){
    uint slice = 0;
    pwm_config c = pwm_get_default_config();
    pwm_config_set_clkdiv(&c, 256.0f);
    pwm_init(slice, &c, false);
    pwm_set_wrap(slice, (125000000/(256*60))-1);
    pwm_clear_irq(slice);
    pwm_set_irq_enabled(slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_isr);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_set_enabled(slice, true);
}


void control_run() {
    init_freq8_table(&control);
    init_song(&control.song);

    irq_set_priority(DMA_IRQ_0,    0);
    irq_set_priority(TIMER_IRQ_0,  1);   // repeating_timer uses hardware alarm on TIMER_IRQ_0
    irq_set_priority(USBCTRL_IRQ,  3);    

    start_60hz_pwm_irq();
    //static repeating_timer_t timer;
    //add_repeating_timer_us(-16667, sequencer_callback, NULL, &timer);
    while (true) {
        tight_loop_contents();  // small wait hint
    }

}    

/*void old_control_run() {
    uint32_t freq8table[128];

    int oct = 4;
    uint16_t vel[NUMBER_OF_VOICES];
    uint16_t pwm[NUMBER_OF_VOICES];
    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        vel[i] = 0;
        pwm[i] = 0;
    }

    int next_voice = 0;
    int waveform = WAV_SAW;
    while (true) {

        tight_loop_contents();  // small wait hint
    }
}*/
