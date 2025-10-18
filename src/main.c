#include "pico/stdlib.h"
#include "hardware/spi.h"

#define PIN_LDAC 16
#define PIN_CS   17
#define PIN_SCK  18   // SPI0 SCK
#define PIN_MOSI 19   // SPI0 MOSI

static inline uint16_t mcp4822_frame(uint8_t ch, bool gain1x, uint16_t v12) {
    v12 &= 0x0FFF;
    // [15]=A/B, [14]=don't care(1), [13]=GA(1=1x), [12]=SHDN(1=active), [11:0]=data
    return ((ch & 1) << 15) | (1 << 14) | ((gain1x ? 1 : 0) << 13) | (1 << 12) | v12;
}

int main() {
    stdio_init_all();

    // --- SPI0 init (mode 0, 1 MHz is plenty) ---
    spi_init(spi0, 1 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // --- CS + LDAC as GPIO ---
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);          // CS idle high

    gpio_init(PIN_LDAC);
    gpio_set_dir(PIN_LDAC, GPIO_OUT);
    gpio_put(PIN_LDAC, 1);        // LDAC idle high (active-low)

    // --- Build frames ---
    uint16_t A = mcp4822_frame(0, true, 0x800);  // ~1.024 V (GA=1x)
    uint16_t B = mcp4822_frame(1, true, 0x400);  // ~0.512 V
    uint8_t txA[2] = { A >> 8, A & 0xFF };
    uint8_t txB[2] = { B >> 8, B & 0xFF };

    // --- Write A then B (CS wraps each 16-bit frame) ---
    gpio_put(PIN_CS, 0); spi_write_blocking(spi0, txA, 2); gpio_put(PIN_CS, 1);
    gpio_put(PIN_CS, 0); spi_write_blocking(spi0, txB, 2); gpio_put(PIN_CS, 1);

    // --- Single LDAC pulse: latch both channels together ---
    gpio_put(PIN_LDAC, 0);
    sleep_us(1);                   // >= 100 ns; 1 us is safe
    gpio_put(PIN_LDAC, 1);

    while (true) tight_loop_contents();  // hold outputs steady
}
