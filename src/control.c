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
} Instr;

Instr instr[NUMBER_OF_VOICES];

uint16_t adsr[NUMBER_OF_VOICES] = {0x039d, 0x395d, 0x0000,0x0000};

static inline void sequencer_callback() {
    dspc_latch();

    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        dspc_set_control(dspc, i, 0x40 | vol[i]);
        dspc_set_pwm(dspc, i, pwm[i]);
        pwm[i]+=instr[i].pwm;
    }

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
            } else if (note < 0) {
                dspc_set_envelope(dspc, i, adsr[i]);
            }
        }
        songpos++;
        step = 0;
    } else {
        step++;
    }
}

static void init_song(Song *song) {
    memset(song, 0, sizeof(Song));
    //int8_t bass_notes[32] = {38,0,0,-1,0,0,38,0,38,50,0,38,50,0,38,0, 34,0,-1,0,46,0,34,0,34,46,0,34,46,0,48,0};
    int8_t mid_notes[32] = {62,0,0,0,0,0,0,-1, 64,0,-1,0,0,0,0,-1,60,0,0,0,0,0,0,-1,67,0,0,-1,69,0,0,-1};
    //memcpy(&song->notes[0], bass_notes, 32);
    memcpy(&song->notes[1], mid_notes, 32);
    /*int8_t mid_notes[32] = {62,0,0,65,0,0,67,0,   62,0,0,60,0,0,62,0, 62,0,0,65,0,0,67,0,   62,0,0,60,0,0,62,0};
    int8_t mid2_notes[32] = {74,0,74,0,74,0,74,0,74,0,74,0,74,0,74,0, 76,0,76,0,76,0,76,0,77,0,77,0,77,0,77,0};
    memcpy(&song->notes[2], mid2_notes, 32);*/
    instr[0].vol = 63;
    instr[0].pwm = 2;
    instr[1].vol = 62;
    instr[1].pwm = 3;
    instr[2].vol = 54;
    instr[2].pwm = 4;
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
