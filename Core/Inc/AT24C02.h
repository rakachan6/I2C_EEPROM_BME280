#ifndef HAL_I2C_AT24C02_H
#define HAL_I2C_AT24C02_H

#include "i2c_driver.h"

#define AT24C02_ADDRESS     (0x50 << 1)
#define AT24C02_SIZE        (256)
#define AT24C02_PAGE_SIZE   (8)
#define AT24C02_PAGES_COUNT (32)
#define AT24C02_WRITE_TIME  (10)
#define AT24C02_MAX_RETRIES (5)
#define AT24C02_BUFFER_SIZE (4)

typedef enum
{
    AT24C02_OK = 0,
    AT24C02_ERROR,
    AT24C02_TIMEOUT,
    AT24C02_OVERFLOW,
}AT24C02_Status_t;

AT24C02_Status_t AT24C02_process(i2c_bus_t * i2c1_bus, int32_t * temp);
#endif //HAL_I2C_AT24C02_H
