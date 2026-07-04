# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (comes from the PICO_SDK_PATH environment variable)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(TrackCall2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(TrackCall2 TrackCall2.c )

pico_generate_pio_header(TrackCall2 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_set_program_name(TrackCall2 "TrackCall2")
pico_set_program_version(TrackCall2 "0.1")

pico_enable_stdio_uart(TrackCall2 1)
pico_enable_stdio_usb(TrackCall2 0)

# Add the standard library to the build
target_link_libraries(TrackCall2 pico_stdlib)

# Add any user requested libraries
target_link_libraries(TrackCall2
    hardware_pio
    hardware_clocks
)
pico_add_extra_outputs(TrackCall2)