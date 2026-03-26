/*
 * MCP23008 based I2C expander
 */

#ifndef __I2C_EXPANDER_H
#define __I2C_EXPANDER_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#ifndef EXP_I2C_PORT
#define EXP_I2C_PORT i2c1
#endif//EXP_I2C_PORT

#ifndef EXP_I2C_ADDR
#define EXP_I2C_ADDR 0x20 // Default address (A0, A1, A2 to GND)
#endif//EXP_I2C_ADDR

// MCP23008 Register Map
#define EXP_I2C_IODIR   0x00 // IO Direction (0=Output, 1=Input)
#define EXP_I2C_GPPU    0x06 // Pull-up Resistors (1=Enabled)
#define EXP_I2C_GPIO    0x09 // Port Register

#ifdef __cplusplus
extern "C"
{
#endif

void i2cExpanderInit(i2c_inst_t* port = EXP_I2C_PORT, uint8_t addr = EXP_I2C_ADDR);
void i2cExpanderWrite(uint8_t val, uint8_t reg = EXP_I2C_GPIO);
void i2cExpanderRead(uint8_t& data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // #ifndef __I2C_EXPANDER_H
