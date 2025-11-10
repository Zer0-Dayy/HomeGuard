#include "main.h"

void HAL_MspInit(void)
{
  /* CubeMX will populate system-wide MSP init (clock, NVIC). */
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  (void)hi2c;
  /* CubeMX generates GPIO/clock configuration here. */
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  (void)huart;
  /* CubeMX generates GPIO/clock configuration here. */
}


