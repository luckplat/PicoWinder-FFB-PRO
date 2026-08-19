cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico CACHE STRING "Board type")

# Run tools/fetch_upstream_unmodified.py first, or manually copy the unchanged
# upstream files listed in vendor/README.md.
include(${CMAKE_CURRENT_LIST_DIR}/vendor/pico_sdk_import.cmake)
project(picowinder_ffb_pro C CXX ASM)
pico_sdk_init()

add_executable(picowinder_ffb_pro
    main.c
    usb.c
    usb_descriptors.c
    ffb_transport.c
    ffb_midi.c
    ffb_engine.c
    input_filter.c
)

target_include_directories(picowinder_ffb_pro PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/vendor
)

pico_generate_pio_header(picowinder_ffb_pro ${CMAKE_CURRENT_LIST_DIR}/vendor/ffb_handshake.pio)
pico_generate_pio_header(picowinder_ffb_pro ${CMAKE_CURRENT_LIST_DIR}/vendor/read_joystick.pio)

pico_enable_stdio_uart(picowinder_ffb_pro 0)
pico_enable_stdio_usb(picowinder_ffb_pro 0)

target_link_libraries(picowinder_ffb_pro PUBLIC
    pico_stdlib
    hardware_pio
    hardware_uart
    tinyusb_device
    tinyusb_board
)

pico_set_program_name(picowinder_ffb_pro "Picowinder FFB PRO")
pico_set_program_version(picowinder_ffb_pro "A22")
pico_add_extra_outputs(picowinder_ffb_pro)
