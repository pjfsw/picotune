cmake_minimum_required(VERSION 3.13)
# Set your board if needed: pico, pico_w, pico2, pico2_w
set(PICO_BOARD pico)  # change to pico2 for RP2350 / Pico 2

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(picotune C CXX ASM)
pico_sdk_init()

add_executable(picotune
    src/main.c
)

pico_generate_pio_header(picotune ${CMAKE_CURRENT_LIST_DIR}/src/mcp4822.pio)

#target_link_libraries(picotune
    #pico_stdlib
    #hardware_pio    
#)

target_link_libraries(picotune pico_stdlib hardware_spi)

pico_enable_stdio_usb(picotune 1)
pico_enable_stdio_uart(picotune 0)
pico_add_extra_outputs(picotune)
