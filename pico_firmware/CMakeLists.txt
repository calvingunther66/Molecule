cmake_minimum_required(VERSION 3.13)

# Initialize the Pico SDK
include(pico_sdk_import.cmake)

project(retrosynth_coprocessor C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(retrosynth_coprocessor
    main.c
    smiles_hash.c
    nn_inference.c
    mcts.c
    smiles_draw.c
)

# Pull in common dependencies
target_link_libraries(retrosynth_coprocessor pico_stdlib hardware_uart)

# Enable usb output, disable uart output since we use UART for Flipper comms
pico_enable_stdio_usb(retrosynth_coprocessor 1)
pico_enable_stdio_uart(retrosynth_coprocessor 0)

# Create map/bin/hex/uf2
pico_add_extra_outputs(retrosynth_coprocessor)
