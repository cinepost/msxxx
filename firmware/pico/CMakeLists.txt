cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH $ENV{HOME}/pico-sdk CACHE STRING "RPI-PICO SDK")

# Initialize Pico SDK
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Pico settings
# Flash Size Configuration (IMPORTANT)
# Set a default flash size of 16384 KB (16MB) if not already specified by the user. If using a standard Pico, change to 2048 (2MB).
set(PICO_FLASH_SIZE_KB 16384 CACHE STRING "Flash size in KB")
set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico)

project(pico_fpga_loader C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Define your executable and source files
include_directories(
  ${PROJECT_SOURCE_DIR}/src
)

add_executable(pico_fpga_loader
    src/main.cpp
    # LittleFS source files
    src/littlefs/lfs.c
    src/littlefs/lfs_util.c
	# SSD1306 source files
    src/ssd1306/ssd1306_i2c.c
)

# Pull in required Pico libraries
# hardware_flash: for LittleFS storage
# hardware_spi:   for FPGA bitstream loading
# hardware_i2c:   for OLED screen
target_link_libraries(pico_fpga_loader 
    pico_stdlib
    pico_multicore 
    hardware_spi 
    hardware_i2c 
    hardware_flash
)

# Enable USB and UART output for debugging
pico_enable_stdio_usb(pico_fpga_loader 1)
pico_enable_stdio_uart(pico_fpga_loader 1)

# Generate UF2, ELF, and BIN files
pico_add_extra_outputs(pico_fpga_loader)
