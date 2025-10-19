#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "dsp.h"
#include "control.h"
#include <stdio.h>

void core1_entry() {
    // DSP run on Core 1
    dsp_run();
}

int main() {
    stdio_init_all();

    sleep_ms(1000);  // give USB time to enumerate

    // Wait until a terminal (picocom, minicom, etc.) connects
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }
    printf("Serial connected, starting up...\n");

    control_init();
    printf("Control init done...\n");

    multicore_launch_core1(core1_entry);

    printf("Core 1 started...\n");
    // Control run on Core 0
    control_run();
}
