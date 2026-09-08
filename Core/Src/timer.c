#include "timer.h"

void delay_us(const i2c_bus_t * bus, const uint32_t us)
{
    __HAL_TIM_SET_COUNTER(bus->delay_tim, 0);
    while (__HAL_TIM_GET_COUNTER(bus->delay_tim) < us)
    {}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)   /* отдельный таймер на 1 Гц, не TIM10 */
        measure_due = 1;
}
