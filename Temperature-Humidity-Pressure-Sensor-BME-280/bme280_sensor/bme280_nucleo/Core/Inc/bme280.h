#ifndef BME280_H
#define BME280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define BME280_I2C_ADDR_DEFAULT (0x76U << 1) /* Shifted for HAL */

typedef struct
{
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
  uint8_t dig_H1;
  int16_t dig_H2;
  uint8_t dig_H3;
  int16_t dig_H4;
  int16_t dig_H5;
  int8_t dig_H6;
} BME280_CalibData_t;

typedef struct
{
  I2C_HandleTypeDef *i2c;
  uint8_t address;
  BME280_CalibData_t calib;
  int32_t t_fine;
} BME280_Handle_t;

typedef struct
{
  float temperature_c;
  float humidity_rh;
  float pressure_pa;
} BME280_Measurement_t;

HAL_StatusTypeDef BME280_Init(BME280_Handle_t *dev);
HAL_StatusTypeDef BME280_ReadMeasurements(BME280_Handle_t *dev, BME280_Measurement_t *out);

#endif /* BME280_H */


