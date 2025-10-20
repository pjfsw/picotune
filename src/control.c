#include <math.h>
#include "control.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "samplerate.h"
#include "dsp_param.h"
#include "wavetable.h"

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


void control_init() {
    dsp_param.volume = 0;
    //wavetable_init();
    get_wavetable_for_frequency(440, &dsp_param);
}

#define VOLUME_STEPS 64

void control_run() {
    //init_sine_table();
    //uint8_t sinofs = 0;
    uint32_t  frequency = 8*440*8;
    dsp_param.phase_add =  (uint32_t)((((uint64_t)frequency) << 29) / SAMPLE_RATE);
    //dsp_param.phase_add = 42792969;

    uint32_t freqtable[128];
    for (int i = 0; i < 128; i++) {
        float f = 440.0 * powf(2.0f, (i-69)/12.0f);
        freqtable[i] = (uint32_t)(f * 8.0);
    }


    //static const uint8_t volmap[16] = {
    //0,  2,  3,  4,  6,  8, 11, 16, 23, 32, 45, 64, 90, 128, 181, 255
    //};
    uint16_t volmap[VOLUME_STEPS];
    for (int i = 0; i < VOLUME_STEPS; i++) {
        float dbfs = -(VOLUME_STEPS-i-1)*1;
        float level = 65535.0 * powf(10, dbfs/20.0);
        volmap[i] = (uint16_t)level;
        printf("db: %f lvl %f\n", dbfs, level);
    }

    int oct = 4;
    uint16_t vel = 0;
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch > 0) {
            printf("keypress\n");
            int n = -1;
            switch (ch) {
                case '+':
                    if (oct < 9) {
                        oct++;
                    }
                    break;
                case '-':
                    if (oct > 1) {
                        oct--;
                    }
                    break;
                case 'q':
                    n = 0;
                    break;
                case '2':
                    n = 1;
                    break;
                case 'w':
                    n = 2;
                    break;
                case '3':
                    n = 3;
                    break;
                case 'e':
                    n = 4;
                    break;
                case 'r':
                    n = 5;
                    break;                    
                case '5':
                    n = 6;
                    break;
                case 't':
                    n = 7;
                    break;
                case '6':
                    n = 8;
                    break;
                case 'y':
                    n = 9;
                    break;
                case '7':
                    n = 10;
                    break;
                case 'u':
                    n = 11;
                    break;
                case 'i': 
                    n = 12;
                    break;
                default:
                    break;
            }
            if (n >= 0) {
                uint32_t freq = freqtable[n+12*oct];
                get_wavetable_for_frequency(freq>>3, &dsp_param);         
                vel = 1023;
                dsp_param.phase_add = (uint32_t)((((uint64_t)freq) << 29) / SAMPLE_RATE);
            }
        }
        sleep_ms(1);
        if (vel > 0) {
            vel--;
            dsp_param.volume = volmap[vel>>4]; 
        } else {
            dsp_param.volume = 0;
        }

        tight_loop_contents();  // small wait hint
    }
}
