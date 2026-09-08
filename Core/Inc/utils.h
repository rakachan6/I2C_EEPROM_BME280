#ifndef HAL_I2C_UTILS_H
#define HAL_I2C_UTILS_H

#include "BME280.h"

extern bme280_meas_data_t BME280_meas_data;
extern i2c_bus_t i2c1_bus;

void meas_process(void);

#endif //HAL_I2C_UTILS_H
