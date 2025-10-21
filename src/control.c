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

//f=440⋅2(n−69)/12

extern volatile DspParam dsp_param[];

void control_init() {
    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        get_wavetable_for_frequency(440, &dsp_param[i]);
        dsp_param[i].phase_add = 0;
        dsp_param[i].volume = 0;
        dsp_param[i].control_id = 0;
    }
}

#define VOLUME_STEPS 64

int get_note_from_key(int ch) {
    int n = -1;
    switch (ch) {
        case 'z':
            n = 0;
            break;
        case 's':
            n = 1;
            break;
        case 'x':
            n = 2;
            break;
        case 'd':
            n = 3;
            break;
        case 'c':
            n = 4;
            break;
        case 'v':
            n = 5;
            break;
        case 'g':
            n = 6;
            break;
        case 'b':
            n = 7;
            break;
        case 'h':
            n = 8;
            break;
        case 'n':
            n = 9;
            break;
        case 'j':
            n = 10;
            break;
        case 'm':
            n = 11;
            break;
        case ',':
        case 'q':
            n = 12;
            break;
        case 'l':        
        case '2':
            n = 13;
            break;
        case '.':
        case 'w':
            n = 14;
            break;
        case '3':
            n = 15;
            break;
        case 'e':
            n = 16;
            break;
        case 'r':
            n = 17;
            break;
        case '5':
            n = 18;
            break;
        case 't':
            n = 19;
            break;
        case '6':
            n = 20;
            break;
        case 'y':
            n = 21;
            break;
        case '7':
            n = 22;
            break;
        case 'u':
            n = 23;
            break;
        case 'i':
            n = 24;
            break;
        case '9':
            n = 25;
            break;
        case 'o':
            n = 26;
            break;
        case '0':
            n = 27;
            break;
        case 'p':
            n = 28;
            break;
        default:
            break;
    }
    return n;
}


void control_run() {
    uint32_t freqtable[128];
    for (int i = 0; i < 128; i++) {
        float f = 440.0 * powf(2.0f, (i-69)/12.0f);
        freqtable[i] = (uint32_t)(f * 8.0);
    }

    uint16_t volmap[VOLUME_STEPS];
    for (int i = 0; i < VOLUME_STEPS; i++) {
        float dbfs = -(VOLUME_STEPS-i-1)*1;
        float level = 65535.0 * powf(10, dbfs/20.0);
        volmap[i] = (uint16_t)level;
        printf("db: %f lvl %f\n", dbfs, level);
    }

    int oct = 4;
    uint16_t vel[NUMBER_OF_VOICES];
    uint16_t pwm[NUMBER_OF_VOICES];
    for (int i = 0; i < NUMBER_OF_VOICES; i++) {
        vel[i] = 0;
        pwm[i] = 0;
    }

    int next_voice = 0;
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch > 0) {
            printf("keypress\n");
            int n = get_note_from_key(ch);
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
                case ' ':
                    for (int i = 0 ; i < NUMBER_OF_VOICES; i++) {                        
                        dsp_param[i].highpass = !dsp_param[i].highpass;
                        bool on = dsp_param[i].highpass;
                        dsp_param[i].control_id++;
                        printf("Highpass: %s\n", on ? "on" : "off");
                    }
                    break;
            }

            if (n >= 0) {
                uint32_t freq = freqtable[n+12*oct];
                get_wavetable_for_frequency(freq>>3, &dsp_param[next_voice]);         
                vel[next_voice] = 2047;
                
                pwm[next_voice] = 16384;
                dsp_param[next_voice].phase_diff = pwm[next_voice]>>2;
                dsp_param[next_voice].phase_add = (uint32_t)((((uint64_t)freq) << 29) / SAMPLE_RATE);
                dsp_param[next_voice].volume = 63;
                dsp_param[next_voice].control_id++;                
                next_voice = (next_voice + 1) % NUMBER_OF_VOICES;
            }
        }
        sleep_ms(1);
        for (int i = 0 ; i < NUMBER_OF_VOICES; i++) {
            if (vel[i] > 0) {
                vel[i]--;
                pwm[i]++;
                dsp_param[i].volume = volmap[vel[i]>>5]; 
                dsp_param[i].phase_diff = pwm[i]>>2;
            } else {
                dsp_param[i].volume = 0;
            }
            dsp_param[i].control_id++;
        }

        tight_loop_contents();  // small wait hint
    }
}
