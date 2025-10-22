Picotune
========

A Raspberry Pie Pico based sound chip.

Recipe
------

- Raspberry Pi Pico
- MCP4822 12-bit SPI DAC
- Some analog spices
- Audio awesomeness

Preliminary specs
-----------------
- 4 synth voices
- PWM square, sawtooth, triangle or noise per voice
- 64 volume levels per voice (1 dB steps)
- Optional highpass filter @ 120 Hz per voice
- Optional lowpass filter TBD

Circuit
-------

Basic concept: Pico connected to DAC, connected to a DC-blocking circuit, connected to a ~16 kHz 6 dB/oct lowpass filter


    [Raspberry Pi Pico] -- GPIO 17 ------ CS -- [MCP4822] -- Out --- (+)10 uF(-) -+----- 1 kOhm ----+------ Lineout   
                           GPIO 18 ------ SCK                                     |                 |
                           GPIO 19 ------ SDI                                  4.7kOhm             10nF
                                                                                  |                 |
                                                                                 Gnd               Gnd

                 


