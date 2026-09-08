#ifndef HAL_I2C_TIME_H
#define HAL_I2C_TIME_H

#include "i2c_driver.h"

extern volatile uint8_t measure_due;

void delay_us(const i2c_bus_t * bus, const uint32_t us);

#endif //HAL_I2C_TIME_H
