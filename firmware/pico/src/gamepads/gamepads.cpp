#include "gamepads.h"

GameControllersModuleConfig g_gpads_conf;

void init_gpads_spi_for_registers() {
    // Initialize SPI at 1MHz (Safe for CD4021B, 74HC597 can go much faster).
    // We can probe for connected gamepads board here to determine exact speed.
    spi_init(SPI_PORT, 1000 * 1000); 
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    // Set Format: 8 bits, CPOL=0, CPHA=0 (Mode 0)
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}