#include "i2c_driver.h"
#include "timer.h"
#include <stdio.h>

/* Реестр нужен колбэкам HAL: они получают I2C_HandleTypeDef *,
   а состояние живёт в i2c_bus_t. */
static i2c_bus_t * i2c_buses[I2C_MAX_BUSES];

static i2c_bus_t * i2c_bus_by_handle(const I2C_HandleTypeDef * hal)
{
    for (uint32_t i = 0U; i < I2C_MAX_BUSES; i++)
    {
        if ((i2c_buses[i] != NULL) && (i2c_buses[i]->hal == hal))
            return i2c_buses[i];
    }
    return NULL;
}

i2c_status_t i2c_bus_init(i2c_bus_t * bus)
{
    if ((bus == NULL) || (bus->hal == NULL) || (bus->delay_tim == NULL) ||
        (bus->scl_port == NULL) || (bus->sda_port == NULL))
        return I2C_ERROR_BUS;

    bus->state       = I2C_IDLE;
    bus->last_result = I2C_RESULT_OK;

    for (uint32_t i = 0U; i < I2C_MAX_BUSES; i++)
    {
        if ((i2c_buses[i] == NULL) || (i2c_buses[i] == bus))
        {
            i2c_buses[i] = bus;
            return I2C_OK;
        }
    }
    return I2C_ERROR_BUS;
}

static void i2c_error_log(const uint32_t error_code, const i2c_op_type_t op_type)
{
    static const char * const op_names[] = {"TRANSMIT", "RECEIVE", "MEM WRITE", "MEM READ"};

    if ((unsigned)op_type >= (sizeof(op_names) / sizeof(op_names[0])))
    {
        printf("I2C error. Undefined operation type. HAL error code is %lu\r\n", error_code);
        return;
    }
    printf("I2C error. Operation type is %s. HAL error code is %lu\r\n",
           op_names[op_type], error_code);
}

static void i2c_try_start(i2c_bus_t * bus)
{
    //Safe atomic I2C bus acquisition_________________________________________________
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (bus->state != I2C_IDLE)
    {
        __set_PRIMASK(primask);
        return;
    }
    bus->state = I2C_BUSY;
    __set_PRIMASK(primask);
    //________________________________________________________________________________

    i2c_transfer_t * t = &bus->current;
    HAL_StatusTypeDef status;
    switch (t->op_type)
    {
    case I2C_OP_TRANSMIT_DMA:
        status = HAL_I2C_Master_Transmit_DMA(bus->hal, t->dev_addr, t->data, t->len);
        break;
    case I2C_OP_RECEIVE_DMA:
        status = HAL_I2C_Master_Receive_DMA(bus->hal, t->dev_addr, t->data, t->len);
        break;
    case I2C_OP_MEM_WRITE_DMA:
        status = HAL_I2C_Mem_Write_DMA(bus->hal, t->dev_addr, t->mem_addr,
                                       I2C_MEMADD_SIZE_8BIT, t->data, t->len);
        break;
    case I2C_OP_MEM_READ_DMA:
        status = HAL_I2C_Mem_Read_DMA(bus->hal, t->dev_addr, t->mem_addr,
                                      I2C_MEMADD_SIZE_8BIT, t->data, t->len);
        break;
    default:
        bus->state = I2C_ERROR;
        return;
    }

    if (status != HAL_OK)
        bus->state = I2C_ERROR;
}

/* Переводит SCL и SDA в open-drain выходы, отпущенные в высокий уровень.
   Читать SDA можно и в этом режиме: при открытом стоке IDR показывает
   реальный уровень линии, поэтому переключать режим по ходу не нужно. */
static void i2c_gpio_release(const i2c_bus_t * bus)
{
    GPIO_InitTypeDef gpio = {0};

    /* Сначала уровень, потом режим: HAL_GPIO_Init не трогает ODR,
       и остаток от прошлой конфигурации дёрнул бы линию вниз. */
    HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(bus->sda_port, bus->sda_pin, GPIO_PIN_SET);

    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = bus->scl_pin;
    HAL_GPIO_Init(bus->scl_port, &gpio);

    gpio.Pin = bus->sda_pin;
    HAL_GPIO_Init(bus->sda_port, &gpio);
}

static void i2c_gpio_restore(const i2c_bus_t * bus)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = bus->gpio_af;

    gpio.Pin = bus->scl_pin;
    HAL_GPIO_Init(bus->scl_port, &gpio);

    gpio.Pin = bus->sda_pin;
    HAL_GPIO_Init(bus->sda_port, &gpio);
}

static void i2c_bus_recovery(const i2c_bus_t * bus)
{
    HAL_I2C_DeInit(bus->hal);
    i2c_gpio_release(bus);

    /* До девяти тактов, пока ведомый не отпустит SDA. */
    for (uint8_t i = 0U; i < 9U; i++)
    {
        if (HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin) == GPIO_PIN_SET)
            break;
        HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_RESET);
        delay_us(bus, 5U);
        HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_SET);
        delay_us(bus, 5U);
    }

    /* STOP: SCL вниз -> SDA вниз -> SCL вверх -> SDA вверх.
       Опускать SDA при высоком SCL нельзя — это условие START. */
    HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_RESET);
    delay_us(bus, 5U);
    HAL_GPIO_WritePin(bus->sda_port, bus->sda_pin, GPIO_PIN_RESET);
    delay_us(bus, 5U);
    HAL_GPIO_WritePin(bus->scl_port, bus->scl_pin, GPIO_PIN_SET);
    delay_us(bus, 5U);
    HAL_GPIO_WritePin(bus->sda_port, bus->sda_pin, GPIO_PIN_SET);
    delay_us(bus, 5U);

    i2c_gpio_restore(bus);
    HAL_I2C_Init(bus->hal);
}

static uint8_t is_i2c_bus_stuck(const i2c_bus_t * bus)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin  = bus->sda_pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(bus->sda_port, &gpio);

    delay_us(bus, 10U);
    GPIO_PinState sda_level = HAL_GPIO_ReadPin(bus->sda_port, bus->sda_pin);

    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = bus->gpio_af;
    HAL_GPIO_Init(bus->sda_port, &gpio);

    return (sda_level == GPIO_PIN_RESET) ? 1U : 0U;
}

static void i2c_error_process(i2c_bus_t * bus)
{
    if (bus->state != I2C_ERROR)
        return;

    i2c_error_log(HAL_I2C_GetError(bus->hal), bus->current.op_type);

    bus->current.retry_count++;
    if (bus->current.retry_count > I2C_MAX_RETRIES)
    {
        printf("I2C transfer dropped after %u retries\r\n", (unsigned)I2C_MAX_RETRIES);
        bus->last_result = I2C_RESULT_FAIL;
        bus->state       = I2C_IDLE;
        return;                       /* повторять больше нечего */
    }

    if (is_i2c_bus_stuck(bus))
    {
        bus->state = I2C_RECOVERING;
        i2c_bus_recovery(bus);
    }
    bus->state = I2C_IDLE;
    i2c_try_start(bus);               /* повтор той же передачи */
}

static i2c_wait_t i2c_wait_ready(i2c_bus_t * bus, const uint16_t len)
{
    const uint32_t start   = HAL_GetTick();
    const uint32_t timeout = (uint32_t)len + I2C_TIMEOUT_BASE_MS;

    /* Разность тиков, а не start + timeout: uwTick переполняется через ~49 суток,
       беззнаковое вычитание это переживает, сравнение с суммой — нет. */
    while ((HAL_GetTick() - start) < timeout)
    {
        i2c_error_process(bus);

        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        i2c_state_t  state  = bus->state;
        i2c_result_t result = bus->last_result;
        __set_PRIMASK(primask);

        /* Порядок важен: после исчерпания повторов состояние тоже I2C_IDLE,
           и без этой проверки отказ выглядел бы как успех. */
        if (result == I2C_RESULT_FAIL)
            return I2C_WAIT_MODULE_ERROR;
        if (state == I2C_IDLE)
            return I2C_WAIT_OK;
    }

    /* Таймаут: снять зависшую передачу, иначе DMA продолжит писать
       в буфер вызывающего после его возврата. DeInit отвязывает DMA. */
    HAL_I2C_DeInit(bus->hal);
    HAL_I2C_Init(bus->hal);
    bus->state       = I2C_IDLE;
    bus->last_result = I2C_RESULT_FAIL;
    return I2C_WAIT_TIMEOUT;
}

/* Единая точка запуска: четыре публичные функции отличаются только op_type,
   поэтому тело живёт здесь — так расхождение между ними невозможно. */
static i2c_status_t i2c_run(i2c_bus_t * bus, const i2c_op_type_t op,
                            const uint8_t dev_addr, uint8_t * data,
                            const uint16_t len, const uint8_t mem_addr)
{
    if ((bus == NULL) || (bus->hal == NULL) || (data == NULL) || (len == 0U))
        return I2C_ERROR_BUS;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    bus->current.dev_addr    = dev_addr;
    bus->current.data        = data;
    bus->current.len         = len;
    bus->current.mem_addr    = mem_addr;
    bus->current.op_type     = op;
    bus->current.retry_count = 0U;
    bus->last_result         = I2C_RESULT_OK;

    __set_PRIMASK(primask);

    i2c_try_start(bus);

    switch (i2c_wait_ready(bus, len))
    {
    case I2C_WAIT_OK:      return I2C_OK;
    case I2C_WAIT_TIMEOUT: return I2C_TIMEOUT;
    default:               return I2C_ERROR_BUS;
    }
}

i2c_status_t i2c_transmit(i2c_bus_t * bus, const uint8_t dev_addr,
                          uint8_t * data, const uint16_t len)
{
    return i2c_run(bus, I2C_OP_TRANSMIT_DMA, dev_addr, data, len, 0U);
}

i2c_status_t i2c_receive(i2c_bus_t * bus, const uint8_t dev_addr,
                         uint8_t * data, const uint16_t len)
{
    return i2c_run(bus, I2C_OP_RECEIVE_DMA, dev_addr, data, len, 0U);
}

i2c_status_t i2c_mem_write(i2c_bus_t * bus, const uint8_t dev_addr,
                           uint8_t * data, const uint16_t len, const uint8_t mem_addr)
{
    return i2c_run(bus, I2C_OP_MEM_WRITE_DMA, dev_addr, data, len, mem_addr);
}

i2c_status_t i2c_mem_read(i2c_bus_t * bus, const uint8_t dev_addr,
                          uint8_t * data, const uint16_t len, const uint8_t mem_addr)
{
    return i2c_run(bus, I2C_OP_MEM_READ_DMA, dev_addr, data, len, mem_addr);
}

static void i2c_transfer_complete(I2C_HandleTypeDef * hal)
{
    i2c_bus_t * bus = i2c_bus_by_handle(hal);
    if (bus != NULL)
        bus->state = I2C_IDLE;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_transfer_complete(hi2c);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_transfer_complete(hi2c);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_transfer_complete(hi2c);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_transfer_complete(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_bus_t * bus = i2c_bus_by_handle(hi2c);
    if (bus != NULL)
        bus->state = I2C_ERROR;
}
