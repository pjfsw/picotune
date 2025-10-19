#include <math.h>
#include "control.h"
#include "pico/stdlib.h"
#include "samplerate.h"

#define TABLE_SIZE 256
static uint8_t sine_table[TABLE_SIZE];
static void init_sine_table(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        float x = (float)i / TABLE_SIZE;
        float s = sinf(2.0f * M_PI * x);        // -1.0 → +1.0
        sine_table[i] = (uint8_t)(128 + s * (63 - 1));
    }
}

volatile uint8_t g_volume = 127;

void control_run() {
    init_sine_table();
    uint8_t sinofs = 0;

    while (true) {
        g_volume = sine_table[sinofs++];
        sleep_ms(5);
    }
}
