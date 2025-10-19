#include <math.h>
#include "control.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "samplerate.h"
#include "dsp_param.h"

/*
#define TABLE_SIZE 256
static uint8_t sine_table[TABLE_SIZE];
static void init_sine_table(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        float x = (float)i / TABLE_SIZE;
        float s = sinf(2.0f * M_PI * x);        // -1.0 → +1.0
        sine_table[i] = (uint8_t)(128 + s * (63 - 1));
    }
}
*/
volatile DspParam dsp_param;

//f=440⋅2(n−69)/12

void control_run() {
    //init_sine_table();
    //uint8_t sinofs = 0;
    dsp_param.volume = 255;
    uint32_t  frequency = 440*8;
    //dsp_param.phase_add =  (uint32_t)((((uint64_t)frequency) << 29) / SAMPLE_RATE);
    //dsp_param.phase_add = 42792969;

    uint32_t freqtable[12];
    for (int i = 0; i < 12; i++) {
        float f = 440.0 * pow(2, (i+69)/12);
        freqtable[i] = (uint32_t)(f * 8);
    }


    while (true) {
        int ch = getchar_timeout_us(0);
        switch (ch) {
            default:
                break;                
            }
        tight_loop_contents();  // small wait hint
    }
}
