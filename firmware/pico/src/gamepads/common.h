/*
 * Gamepads section common logic
 */

#ifndef __GAMEPADS_COMMON_H
#define __GAMEPADS_POLLING_H

#include "hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t poll_74HC597(uint stcp_pin, uint pl_pin) {
    uint8_t buffer;

    // Latch inputs into internal storage
    gpio_put(stcp_pin, 1);
    sleep_us(1);
    gpio_put(stcp_pin, 0);

    // Transfer storage to shift register
    gpio_put(pl_pin, 0); // Active low
    sleep_us(1);
    gpio_put(pl_pin, 1);

    // Read 1 byte via SPI hardware
    spi_read_blocking(SPI_PORT, 0, &buffer, 1);
    return buffer;
}

uint8_t poll_CD4021B(uint ps_pin) {
    uint8_t buffer;

    // Jam-load inputs (Parallel mode)
    gpio_put(ps_pin, 1);
    sleep_us(5); // Slower pulse for CD4000 logic
    gpio_put(ps_pin, 0);

    // Read 1 byte via SPI hardware
    spi_read_blocking(SPI_PORT, 0, &buffer, 1);
    return buffer;
}

// 25CS320(25cs640) board eeprom 
uint8_t read_board_id(uint cs_pin) {
	uint8_t read_security_cmd = 0x83; // Example opcode for 25CS series
	uint8_t address[2] = {0x00, 0x00}; // Starting address of security register
	uint8_t serial_number[16]; // To hold the 128-bit ID

	gpio_put(cs_pin, 0);
	spi_write_blocking(spi0, &read_security_cmd, 1);
	spi_write_blocking(spi0, address, 2);
	spi_read_blocking(spi0, 0, serial_number, 16);
	gpio_put(cs_pin, 1);

	// TODO: read board id
	uint8_t board_id;

	return board_id;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // __GAMEPADS_POLLING_H