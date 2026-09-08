#ifndef HAL_I2C_BME280_REG_H
#define HAL_I2C_BME280_REG_H

#define BME280_CALIB_1_ADDR      ((uint8_t) 0x88)
#define BME280_CALIB_2_ADDR      (0xE1)

#define BME280_ID_ADDR          (0xD0)
#define BME280_RESET_ADDR       (0xE0)
#define BME280_CTRL_HUM_ADDR    (0xF2)
#define BME280_STATUS_ADDR      (0xF3)
#define BME280_CTRL_MEAS_ADDR   (0xF4)
#define BME280_CONFIG_ADDR      (0xF5)
#define BME280_PRESS_ADDR       (0xF7)
#define BME280_TEMP_ADDR        (0xFA)
#define BME280_HUM_ADDR         (0xFD)

#define BME280_ID_VALUE         (0x60)
#define BME280_RESET_VALUE      (0xB6)

#define BME280_HUM_OSRS_MSK     (0x07)
#define BME280_HUM_OSRS_0       (0x00)
#define BME280_HUM_OSRS_1       (0x01)
#define BME280_HUM_OSRS_2       (0x02)
#define BME280_HUM_OSRS_4       (0x03)
#define BME280_HUM_OSRS_8       (0x04)
#define BME280_HUM_OSRS_16      (0x05)

#define BME280_TEMP_OSRS_MSK    (0x07 << 5)
#define BME280_TEMP_OSRS_0      (0x00 << 5)
#define BME280_TEMP_OSRS_1      (0x01 << 5)
#define BME280_TEMP_OSRS_2      (0x02 << 5)
#define BME280_TEMP_OSRS_4      (0x03 << 5)
#define BME280_TEMP_OSRS_8      (0x04 << 5)
#define BME280_TEMP_OSRS_16     (0x05 << 5)

#define BME280_PRESS_OSRS_MSK   (0x07 << 2)
#define BME280_PRESS_OSRS_0     (0x00 << 2)
#define BME280_PRESS_OSRS_1     (0x01 << 2)
#define BME280_PRESS_OSRS_2     (0x02 << 2)
#define BME280_PRESS_OSRS_4     (0x03 << 2)
#define BME280_PRESS_OSRS_8     (0x04 << 2)
#define BME280_PRESS_OSRS_16    (0x05 << 2)

#define BME280_MODE_MSK         (0x03)
#define BME280_MODE_FORCE       (0x01)
#define BME280_MODE_NORMAL      (0x03)

#define BME280_TIME_MSK         (0xE0)
#define BME280_TIME_0_5MS       (0x00)
#define BME280_TIME_10MS        (0xC0)
#define BME280_TIME_20MS        (0xE0)
#define BME280_TIME_65_5MS      (0x20)
#define BME280_TIME_125MS       (0x40)
#define BME280_TIME_250MS       (0x60)
#define BME280_TIME_500MS       (0x80)
#define BME280_TIME_1000MS      (0xA0)

#define BME280_FILTER_MSK       (0x1C)
#define BME280_FILTER_OFF       (0x00)
#define BME280_FILTER_2         (0x04)
#define BME280_FILTER_4         (0x08)
#define BME280_FILTER_8         (0x0C)
#define BME280_FILTER_16        (0x10)

#define BME280_MEAS_PROCESS     (0x01)
#define BME280_UPDATING         (0x08)

#endif //HAL_I2C_BME280_REG_H
