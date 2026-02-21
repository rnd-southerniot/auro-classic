cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(airborne_classic_fw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(airborne_classic_fw
    src/main.c
)

target_link_libraries(airborne_classic_fw
    pico_stdlib
    hardware_pio
    hardware_irq
)

pico_enable_stdio_uart(airborne_classic_fw 0)
pico_enable_stdio_usb(airborne_classic_fw 1)

pico_add_extra_outputs(airborne_classic_fw)
