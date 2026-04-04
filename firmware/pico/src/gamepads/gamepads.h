/*
 * Gamepads section common logic
 */

#ifndef __GAMEPADS_GAMEPADS_H
#define __GAMEPADS_GAMEPADS_H

#include "hardware/spi.h"

// SPI pins
#define SPI_PORT spi1
#define PIN_MISO 16 // Data from Shift Register (Q7S/Q8)
#define PIN_SCK  18 // Clock to Shift Register

struct GameControllersModuleConfig {
	
};

#ifdef __cplusplus
extern "C" {
#endif

// Initialize gamepads board SPI.
void init_gpads_spi_for_registers();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // __GAMEPADS_GAMEPADS_H