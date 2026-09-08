#include "main.h"
#include "BME280.h"
#include "BME280_REG.h"
#include <stdio.h>

static const bme280_config_t BME280_config = {
    BME280_MODE_NORMAL,
    BME280_TIME_1000MS,
    BME280_FILTER_OFF,
    BME280_TEMP_OSRS_16,
    BME280_PRESS_OSRS_16,
    BME280_HUM_OSRS_16,
};

bme280_meas_data_t BME280_meas_data = {0};

static int32_t adc_t = 0;
static int32_t adc_p = 0;
static int32_t adc_h = 0;

static uint16_t dig_T1 = 0;
static int16_t dig_T2 = 0;
static int16_t dig_T3 = 0;

static uint16_t dig_P1 = 0;
static int16_t dig_P2 = 0;
static int16_t dig_P3 = 0;
static int16_t dig_P4 = 0;
static int16_t dig_P5 = 0;
static int16_t dig_P6 = 0;
static int16_t dig_P7 = 0;
static int16_t dig_P8 = 0;
static int16_t dig_P9 = 0;

static uint8_t dig_H1 = 0;
static int16_t dig_H2 = 0;
static uint8_t dig_H3 = 0;
static int16_t dig_H4 = 0;
static int16_t dig_H5 = 0;
static int8_t  dig_H6 = 0;

static int32_t t_fine;

static bme280_status_t BME280_read(i2c_bus_t * bus, uint8_t addr,
                                   uint8_t * data, const uint8_t length)
{
    if (i2c_mem_read(bus, BME280_ADDRESS, data, length, addr) != I2C_OK)
        return BME280_ERROR;

    return BME280_OK;
}

static bme280_status_t BME280_get_calib_values(i2c_bus_t * bus)
{
    uint8_t calib_buf_1[26];
    uint8_t calib_buf_2[7];

    if (BME280_read(bus, BME280_CALIB_1_ADDR, calib_buf_1, sizeof(calib_buf_1)) != BME280_OK)
        return BME280_ERROR;
    if (BME280_read(bus, BME280_CALIB_2_ADDR, calib_buf_2, sizeof(calib_buf_2)) != BME280_OK)
        return BME280_ERROR;

    dig_T1 = (uint16_t) ((calib_buf_1[1] << 8) | calib_buf_1[0]);
    dig_T2 = (int16_t) ((calib_buf_1[3] << 8) | calib_buf_1[2]);
    dig_T3 = (int16_t) ((calib_buf_1[5] << 8) | calib_buf_1[4]);

    dig_P1 = (uint16_t) ((calib_buf_1[7] << 8) | calib_buf_1[6]);
    dig_P2 = (int16_t) ((calib_buf_1[9] << 8) | calib_buf_1[8]);
    dig_P3 = (int16_t) ((calib_buf_1[11] << 8) | calib_buf_1[10]);
    dig_P4 = (int16_t) ((calib_buf_1[13] << 8) | calib_buf_1[12]);
    dig_P5 = (int16_t) ((calib_buf_1[15] << 8) | calib_buf_1[14]);
    dig_P6 = (int16_t) ((calib_buf_1[17] << 8) | calib_buf_1[16]);
    dig_P7 = (int16_t) ((calib_buf_1[19] << 8) | calib_buf_1[18]);
    dig_P8 = (int16_t) ((calib_buf_1[21] << 8) | calib_buf_1[20]);
    dig_P9 = (int16_t) ((calib_buf_1[23] << 8) | calib_buf_1[22]);

    dig_H1 = calib_buf_1[25];
    dig_H2 = (int16_t) ((calib_buf_2[1] << 8) | calib_buf_2[0]);
    dig_H3 = calib_buf_2[2];
    dig_H4 = (int16_t)((int8_t)calib_buf_2[3] * 16) | (int16_t)(calib_buf_2[4] & 0x0F);
    dig_H5 = (int16_t)((int8_t)calib_buf_2[5] * 16) | (int16_t)(calib_buf_2[4] >> 4);
    dig_H6 = (int8_t)calib_buf_2[6];

    return BME280_OK;
}

static bme280_status_t BME280_reset(i2c_bus_t * bus)
{
    uint8_t reset_value = BME280_RESET_VALUE;

    if (i2c_mem_write(bus, BME280_ADDRESS, &reset_value, 1, BME280_RESET_ADDR) != I2C_OK)
        return BME280_ERROR;

    return BME280_OK;
}

//to print only the measured characteristics
static void bme280_write_printf_config(void)
{
    if (BME280_config.temp_osrs)
        BME280_meas_data.print_config |= BME280_PRINT_TEMP_ON;
    if (BME280_config.pressure_osrs)
        BME280_meas_data.print_config |= BME280_PRINT_PRESS_ON;
    if (BME280_config.humidity_osrs)
        BME280_meas_data.print_config |= BME280_PRINT_HUM_ON;
}

bme280_status_t BME280_init(i2c_bus_t * bus)
{
    uint8_t bme280_status;
    uint8_t config_buffer[6] = {
        BME280_CTRL_HUM_ADDR, BME280_config.humidity_osrs,
        BME280_CTRL_MEAS_ADDR, (BME280_config.mode | BME280_config.pressure_osrs | BME280_config.temp_osrs),
        BME280_CONFIG_ADDR, (BME280_config.filter | BME280_config.time)};
    uint8_t control_buffer[3];
    uint8_t id;

    //check ID
    if (BME280_read(bus, BME280_ID_ADDR, &id, 1) != BME280_OK)
    {
        printf("BME280 ID read error\r\n");
        return BME280_ERROR;
    }
    if (id != BME280_ID_VALUE)
    {
        printf("BME280 WRONG ID\r\n");
        return BME280_ID_WRONG;
    }

    //write configuration
    if (i2c_transmit(bus, BME280_ADDRESS, config_buffer, sizeof(config_buffer)) != I2C_OK)
    {
        printf("BME280 config tx error\r\n");
        return BME280_ERROR;
    }
    HAL_Delay(100);

    //is data copied check
    if (BME280_read(bus, BME280_STATUS_ADDR, &bme280_status, 1) != BME280_OK)
    {
        printf("BME280 config read error\r\n");
        return BME280_ERROR;
    }
    if ((bme280_status & BME280_UPDATING) == BME280_UPDATING)
    {
        printf("BME280 updating error\r\n");
        BME280_reset(bus);
        return BME280_ERROR;
    }

    //read config registers
    if (BME280_read(bus, BME280_CTRL_HUM_ADDR, control_buffer, 1) != BME280_OK)
    {
        printf("BME280 read hum config error\r\n");
        return BME280_ERROR;
    }
    if (BME280_read(bus, BME280_CTRL_MEAS_ADDR, &control_buffer[1], 2) != BME280_OK)
    {
        printf("BME280 read ctrl meas config error\r\n");
        return BME280_ERROR;
    }

    //check control registers
    for (uint8_t i = 0; i < sizeof(control_buffer); i++)
    {
        if (control_buffer[i] != config_buffer[i * 2 + 1])
        {
            printf("BME280 config buffer does not match\r\n");
            return BME280_ERROR;
        }
    }

    if (BME280_get_calib_values(bus) != BME280_OK)
    {
        printf("BME280 get calib values error\r\n");
        return BME280_ERROR;
    }

    bme280_write_printf_config();

    return BME280_OK;
}

static int32_t BME280_compensate_T(int32_t adc_t)
{
    int32_t var1, var2, T;
    var1 = ((((adc_t >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_t >> 4) - ((int32_t)dig_T1)) * ((adc_t >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t BME280_compensate_P(int32_t adc_p)
{
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)dig_P6;
    var2 = var2 + ((var1 * ((int32_t) dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t) dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);
    //avoid exception caused division by zero
    if (var1 == 0)
        return 0;
    p = (((uint32_t)(((int32_t)1048576) - adc_p) - (var2 >> 12))) * 3125;
    if (p < 0x80000000)
        p = (p << 1) / ((uint32_t)var1);
    else
        p = (p / (uint32_t) var1) * 2;
    var1 = (((int32_t) dig_P9) * ((int32_t) (((p >> 3)) * (p >> 3) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
    return p;
}

static uint32_t BME280_compensate_H(int32_t adc_h)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_h << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) *
        v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r *
        ((int32_t)dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) +
        ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) +
        8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
        ((int32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    return (uint32_t) (v_x1_u32r >> 12);
}

static bme280_status_t BME280_read_meas(i2c_bus_t * bus)
{
    uint8_t buff[8] = {0};

    if (BME280_read(bus, BME280_PRESS_ADDR, buff, sizeof(buff)) != BME280_OK)
        return BME280_ERROR;
    adc_p = ((int32_t)buff[0] << 12) | ((int32_t)buff[1] << 4) | (buff[2] >> 4);
    adc_t = ((int32_t)buff[3] << 12) | ((int32_t)buff[4] << 4) | ((int32_t)buff[5] >> 4);
    adc_h = ((int32_t)buff[6] << 8) | (int32_t)buff[7];
    if (BME280_config.temp_osrs)
        BME280_meas_data.temp = BME280_compensate_T(adc_t);
    if (BME280_config.pressure_osrs)
        BME280_meas_data.press = BME280_compensate_P(adc_p);
    if (BME280_config.humidity_osrs)
        BME280_meas_data.hum = BME280_compensate_H(adc_h);
    return BME280_OK;
}

static bme280_status_t BME280_meas_range_check(void)
{
    if ((BME280_meas_data.temp < BME280_MIN_T) || (BME280_meas_data.temp > BME280_MAX_T))
    {
        printf("BME280 temperature out of range\r\n");
        return BME280_ERROR;
    }
    if ((BME280_meas_data.press < BME280_MIN_P) || (BME280_meas_data.press > BME280_MAX_P))
    {
        printf("BME280 pressure out of range\r\n");
        return BME280_ERROR;
    }
    //lower limit is 0
    if (BME280_meas_data.hum > BME280_MAX_H)
    {
        printf("BME280 humidity out of range\r\n");
        return BME280_ERROR;
    }
    return BME280_OK;
}

bme280_status_t BME280_process(i2c_bus_t * i2c1_bus)
{

    static uint8_t get_meas_retries = 0;

    if (BME280_read_meas(i2c1_bus) != BME280_OK)
    {
        printf("BME280 read measurements error\r\n");
        get_meas_retries++;
        if (get_meas_retries > BME280_GET_MEAS_MAX_RETRIES)
        {
            printf("BME280 read measurements error > %d\r\n", BME280_GET_MEAS_MAX_RETRIES);
            Error_Handler();
        }
        return BME280_ERROR;
    }
    if (BME280_meas_range_check() != BME280_OK)
    {
        get_meas_retries++;
        if (get_meas_retries > BME280_GET_MEAS_MAX_RETRIES)
        {
            printf("BME280 measurements error > %d\r\n", BME280_GET_MEAS_MAX_RETRIES);
            Error_Handler();
        }
        return BME280_ERROR;
    }
    else
        get_meas_retries = 0;



    return BME280_OK;
}


