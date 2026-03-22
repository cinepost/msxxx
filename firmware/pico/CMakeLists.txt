cmake_minimum_required(VERSION 3.13)

# 1. Initialize Pico SDK
include(pico_sdk_import.cmake)

project(pico_fpga_loader C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# 2. Define your executable and source files
add_executable(pico_fpga_loader
    src/main.cpp
    # Include LittleFS source files
    src/littlefs/lfs.c
    src/littlefs/lfs_util.c
)

# 3. Pull in required Pico libraries
# hardware_flash: for LittleFS storage
# hardware_spi:   for FPGA bitstream loading
# hardware_i2c:   for OLED screen
target_link_libraries(pico_fpga_loader 
    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_flash
)

# 4. Enable USB and UART output for debugging
pico_enable_stdio_usb(pico_fpga_loader 1)
pico_enable_stdio_uart(pico_fpga_loader 1)

# 5. Generate UF2, ELF, and BIN files
pico_add_extra_outputs(pico_fpga_loader)

# 6. Flash Size Configuration (IMPORTANT)
# This tells the linker your board has 16MB of Flash.
# If using a standard Pico, change to 2048 (2MB).
set(PICO_FLASH_SIZE_KB 16384)
