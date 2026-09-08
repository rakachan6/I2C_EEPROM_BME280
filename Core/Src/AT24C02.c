#include "AT24C02.h"
#include "stdio.h"
#include "main.h"
#include <string.h>

static AT24C02_Status_t AT24C02_byte_transmit(i2c_bus_t * i2c1_bus, uint8_t addr, uint8_t data)
{
    i2c_status_t status;
    HAL_StatusTypeDef hal_status;
    status = i2c_mem_write(i2c1_bus, AT24C02_ADDRESS, &data, 1, addr);
    if (status != I2C_OK)
        return AT24C02_ERROR;
    HAL_Delay(AT24C02_WRITE_TIME);
    hal_status = HAL_I2C_IsDeviceReady(i2c1_bus->hal, AT24C02_ADDRESS, 5, 100);
    if (hal_status != HAL_OK)
        return AT24C02_ERROR;
    else
        return AT24C02_OK;
}

static AT24C02_Status_t AT24C02_page_transmit(i2c_bus_t * i2c1_bus, uint8_t addr, uint8_t * data, uint16_t length)
{
    i2c_status_t status;
    HAL_StatusTypeDef hal_status;
    //length more than rest of pege
    if ((addr % AT24C02_PAGE_SIZE) + length > AT24C02_PAGE_SIZE)
        return AT24C02_OVERFLOW;
    status = i2c_mem_write(i2c1_bus, AT24C02_ADDRESS, data, length,addr);
    if (status != I2C_OK)
        return AT24C02_ERROR;
    HAL_Delay(AT24C02_WRITE_TIME);
    hal_status = HAL_I2C_IsDeviceReady(i2c1_bus->hal, AT24C02_ADDRESS, 10, 50);
    if (hal_status != HAL_OK)
        return AT24C02_ERROR;
    else
        return AT24C02_OK;
}

static AT24C02_Status_t AT24C02_transmit_all(i2c_bus_t * i2c1_bus, uint8_t addr, uint8_t * data, uint16_t length)
{
    uint16_t current_length;
    AT24C02_Status_t status;
    if ((uint16_t)addr + length > AT24C02_SIZE)
        return AT24C02_OVERFLOW;
    while (length > 0U)
    {
        current_length = AT24C02_PAGE_SIZE - (addr % AT24C02_PAGE_SIZE);
        if (current_length > length)
            current_length = length;

        status = AT24C02_page_transmit(i2c1_bus, addr, data, current_length);
        if (status != AT24C02_OK)
            return status;

        length -= current_length;
        addr   += current_length;
        data   += current_length;
    }
    return AT24C02_OK;
}

static AT24C02_Status_t AT24C02_receive(i2c_bus_t * i2c1_bus, uint8_t addr, uint8_t * data, uint16_t length)
{
    i2c_status_t status;
    if ((uint16_t)addr + length > AT24C02_SIZE)
        return AT24C02_OVERFLOW;
    status = i2c_mem_read(i2c1_bus, AT24C02_ADDRESS, data, length, addr);
    if (status != I2C_OK)
        return AT24C02_ERROR;
    else
        return AT24C02_OK;
}

static void zip_temp(uint8_t * buffer, const int32_t * temp)
{
    buffer[0] = *temp & 0xFF;
    buffer[1] = (*temp >> 8) & 0xFF;
    buffer[2] = (*temp >> 16) & 0xFF;
    buffer[3] = (*temp >> 24) & 0xFF;
}

static void unzip_temp(const uint8_t * buffer, int32_t * temp)
{
    *temp = (int32_t)((uint32_t)buffer[0]         |
                      ((uint32_t)buffer[1] << 8)  |
                      ((uint32_t)buffer[2] << 16) |
                      ((uint32_t)buffer[3] << 24));
}

AT24C02_Status_t AT24C02_process(i2c_bus_t * i2c1_bus,int32_t * temp)
{
    static uint16_t at24c02_mem_addr = 0;
    static uint8_t at24c02_retries = 0;
    uint8_t buffer[AT24C02_BUFFER_SIZE];
    zip_temp(buffer, temp);
    if (AT24C02_transmit_all(i2c1_bus, at24c02_mem_addr, buffer, AT24C02_BUFFER_SIZE) != AT24C02_OK)
    {
        printf("AT24C02 write temp error\r\n");
        at24c02_retries++;
        if (at24c02_retries > AT24C02_MAX_RETRIES)
        {
            printf("AT24C02 error retries > %d\r\n", AT24C02_MAX_RETRIES);
            Error_Handler();
        }
        return AT24C02_ERROR;
    }
    memset(buffer, 0, AT24C02_BUFFER_SIZE);
    if (AT24C02_receive(i2c1_bus, at24c02_mem_addr, buffer, AT24C02_BUFFER_SIZE) != AT24C02_OK)
    {
        printf("AT24C02 read temp error\r\n");
        at24c02_retries++;
        if (at24c02_retries > AT24C02_MAX_RETRIES)
        {
            printf("AT24C02 error retries > %d\r\n", AT24C02_MAX_RETRIES);
            Error_Handler();
        }
        return AT24C02_ERROR;
    }

    at24c02_retries = 0;
    unzip_temp(buffer, temp);
    at24c02_mem_addr = (at24c02_mem_addr + AT24C02_BUFFER_SIZE) % AT24C02_SIZE;

    return AT24C02_OK;
}