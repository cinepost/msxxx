#include <stdlib.h>

#include "i2c_expander.h"

struct {
    i2c_inst_t* port;
    uint8_t addr;
} g_exp_i2c_conf;

void i2cExpanderInit(i2c_inst_t* port, uint8_t addr) {
    g_exp_i2c_conf.port = port;
    g_exp_i2c_conf.addr = addr;
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Configuration: Pins 0-3 as INPUTS, Pins 4-7 as OUTPUTS
    // Binary: 0000 1111 -> 0x0F
    i2cExpanderWrite(0x0F, EXP_I2C_IODIR);
    
    // Enable Internal Pull-ups for pins 0-3 (Buttons)
    i2cExpanderWrite(0x0F, EXP_I2C_GPPU);
}

void i2cExpanderWrite(uint8_t val, uint8_t reg) {
    uint8_t buf[] = {reg, val};
    i2c_write_blocking(g_exp_i2c_conf.port, g_exp_i2c_conf.addr, buf, 2, false);
}

void i2cExpanderRead(uint8_t& data) {
    static uint8_t reg = EXP_I2C_GPIO;
    i2c_write_blocking(g_exp_i2c_conf.port, g_exp_i2c_conf.addr, &reg, 1, true);
    i2c_read_blocking(g_exp_i2c_conf.port, g_exp_i2c_conf.addr, &data, 1, false);
}