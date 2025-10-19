#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "dsp.h"
#include "control.h"

void core1_entry() {
    // DSP run on Core 1
    dsp_run();
}

int main() {
    stdio_init_all();
    multicore_launch_core1(core1_entry);
    // Control run on Core 0
    control_run();
}
