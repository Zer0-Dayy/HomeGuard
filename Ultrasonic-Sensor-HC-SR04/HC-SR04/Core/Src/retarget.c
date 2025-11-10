#include "main.h"
#include <stdio.h>

int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  extern UART_HandleTypeDef huart3;
  HAL_UART_Transmit(&huart3, &c, 1, HAL_MAX_DELAY);
  return ch;
}

int _write(int file, char *ptr, int len)
{
  (void)file;
  extern UART_HandleTypeDef huart3;
  HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}

