#ifndef HAL_I2C_LED_H
#define HAL_I2C_LED_H

#include "stm32f4xx_hal.h"

#define ERROR_LED_ON() (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET))

#endif //HAL_I2C_LED_H
