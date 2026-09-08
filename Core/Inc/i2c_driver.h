#ifndef HAL_I2C_I2C_DRIVER_H
#define HAL_I2C_I2C_DRIVER_H

#include "stm32f4xx_hal.h"

#define I2C_MAX_RETRIES     (5U)
#define I2C_TIMEOUT_BASE_MS (10U)
#define I2C_MAX_BUSES       (2U)

typedef enum
{
    I2C_WAIT_OK = 0,
    I2C_WAIT_TIMEOUT,
    I2C_WAIT_MODULE_ERROR,
} i2c_wait_t;

typedef enum
{
    I2C_OP_TRANSMIT_DMA = 0,
    I2C_OP_RECEIVE_DMA,
    I2C_OP_MEM_WRITE_DMA,
    I2C_OP_MEM_READ_DMA,
} i2c_op_type_t;

typedef enum
{
    I2C_IDLE = 0,
    I2C_BUSY,
    I2C_ERROR,
    I2C_RECOVERING,
} i2c_state_t;

typedef enum
{
    I2C_OK = 0,
    I2C_TIMEOUT,
    I2C_ERROR_BUS,
} i2c_status_t;

typedef enum
{
    I2C_RESULT_OK = 0,
    I2C_RESULT_FAIL,
} i2c_result_t;

typedef struct
{
    uint8_t         dev_addr;
    uint8_t *       data;
    uint16_t        len;
    uint8_t         mem_addr;
    i2c_op_type_t   op_type;
    uint8_t         retry_count;
} i2c_transfer_t;

/**
 * Описание шины: HAL-хэндл плюс всё, чего в нём нет.
 * HAL не хранит выводы SCL/SDA (их настраивает пользовательская
 * HAL_I2C_MspInit, и эта информация никуда не записывается), поэтому
 * для восстановления зависшей шины пины приходится нести самим.
 *
 * Поля hal..delay_tim заполняет пользователь, current..last_result —
 * внутреннее состояние, его инициализирует i2c_bus_init().
 */
typedef struct
{
    I2C_HandleTypeDef *   hal;
    GPIO_TypeDef *        scl_port;
    uint16_t              scl_pin;
    GPIO_TypeDef *        sda_port;
    uint16_t              sda_pin;
    uint32_t              gpio_af;      /* GPIO_AF4_I2C1 и т.п. */
    TIM_HandleTypeDef *   delay_tim;    /* 1 тик = 1 мкс, должен быть запущен */

    i2c_transfer_t        current;
    volatile i2c_state_t  state;
    volatile i2c_result_t last_result;
} i2c_bus_t;

/**
 * Регистрирует шину и сбрасывает её состояние.
 * Вызывать один раз после MX_I2Cx_Init() и запуска delay_tim,
 * до первого обмена. Без этого колбэки HAL не найдут шину.
 */
i2c_status_t i2c_bus_init(i2c_bus_t * bus);

/*
 * Все четыре функции блокирующие: возвращаются, когда обмен завершён,
 * отброшен после I2C_MAX_RETRIES повторов или истёк таймаут
 * (len + I2C_TIMEOUT_BASE_MS миллисекунд).
 *
 * Буфер data должен оставаться живым до возврата — с ним работает DMA.
 * dev_addr — уже сдвинутый семибитный адрес (0x76 << 1).
 * Из обработчиков прерываний вызывать нельзя.
 */
i2c_status_t i2c_transmit(i2c_bus_t * bus, uint8_t dev_addr,
                          uint8_t * data, uint16_t len);

i2c_status_t i2c_receive(i2c_bus_t * bus, uint8_t dev_addr,
                         uint8_t * data, uint16_t len);

i2c_status_t i2c_mem_write(i2c_bus_t * bus, uint8_t dev_addr,
                           uint8_t * data, uint16_t len, uint8_t mem_addr);

i2c_status_t i2c_mem_read(i2c_bus_t * bus, uint8_t dev_addr,
                          uint8_t * data, uint16_t len, uint8_t mem_addr);

#endif //HAL_I2C_I2C_DRIVER_H
