#include "utils.h"
#include "AT24C02.h"
#include "stdio.h"

static void print_meas(void)
{
    if (BME280_IS_PRINT_TEMP)
    {
        int32_t t = BME280_meas_data.temp;
        const char * sign = (t < 0) ? "-" : "";
        if (t < 0)
            t = -t;
        printf("Temperature = %s%ld.%02ld deg C\r\n", sign, t / 100, t % 100);
    }
    if (BME280_IS_PRINT_PRESS)
    {
        uint32_t mmhg_x100 = (uint32_t)(((uint64_t)BME280_meas_data.press * 76000ULL) / 101325ULL);
        printf("Pressure = %lu.%02lu mmHg\r\n", mmhg_x100 / 100, mmhg_x100 % 100);
    }
    if (BME280_IS_PRINT_HUM)
    {
        uint32_t h = BME280_meas_data.hum;
        printf("Humidity = %ld.%02ld %%\r\n\n",h / 1024, h % 1024);
    }
}

void meas_process(void)
{
    if (BME280_process(&i2c1_bus) == BME280_OK)
    {
        int32_t written = BME280_meas_data.temp;

        if (AT24C02_process(&i2c1_bus, &written) != AT24C02_OK)
        {
            printf("AT24C02 write temperature error\r\n");
            written = BME280_meas_data.temp;
        }

        if (written != BME280_meas_data.temp)
        {
            printf("AT24C02 MISMATCH: wrote %ld, read %ld\r\n", written, BME280_meas_data.temp);
        }
        else
        {
            print_meas();
        }
    }
}