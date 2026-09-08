#ifndef HAL_I2C_BME280_H
#define HAL_I2C_BME280_H

#include "i2c_driver.h"

#define BME280_ADDRESS              (0x76 << 1)
#define BME280_INIT_RETRIES         (5)
#define BME280_GET_MEAS_MAX_RETRIES (5)
#define BME280_MIN_T                (-4000)
#define BME280_MAX_T                (8500)
#define BME280_MIN_P                (30000)
#define BME280_MAX_P                (110000)
#define BME280_MAX_H                (102400)

#define BME280_PRINT_TEMP_ON        (1)
#define BME280_PRINT_PRESS_ON       (2)
#define BME280_PRINT_HUM_ON         (4)
#define BME280_IS_PRINT_TEMP        (BME280_meas_data.print_config & BME280_PRINT_TEMP_ON)
#define BME280_IS_PRINT_PRESS       (BME280_meas_data.print_config & BME280_PRINT_PRESS_ON)
#define BME280_IS_PRINT_HUM         (BME280_meas_data.print_config & BME280_PRINT_HUM_ON)

typedef enum
{
    BME280_OK = 0,
    BME280_ERROR,
    BME280_ID_WRONG,
} bme280_status_t;

typedef struct
{
    uint8_t mode;
    uint8_t time;
    uint8_t filter;
    uint8_t temp_osrs;
    uint8_t pressure_osrs;
    uint8_t humidity_osrs;
} bme280_config_t;

typedef struct
{
    int32_t temp;
    uint32_t press;
    uint32_t hum;
    uint8_t print_config; //for printf
} bme280_meas_data_t;

bme280_status_t BME280_init(i2c_bus_t * bus);
bme280_status_t BME280_process(i2c_bus_t  * i2c1_bus);

#endif //HAL_I2C_BME280_H
