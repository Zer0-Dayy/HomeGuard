#include "bme280.h"
#include <string.h>

#define BME280_REG_ID 0xD0U
#define BME280_REG_RESET 0xE0U
#define BME280_REG_CTRL_HUM 0xF2U
#define BME280_REG_STATUS 0xF3U
#define BME280_REG_CTRL_MEAS 0xF4U
#define BME280_REG_CONFIG 0xF5U
#define BME280_REG_PRESS_MSB 0xF7U

#define BME280_RESET_VALUE 0xB6U

#define BME280_STATUS_MEASURING (1U << 3)
#define BME280_TIMEOUT_MS 100U

static HAL_StatusTypeDef bme280_read_calibration(BME280_Handle_t *dev);
static HAL_StatusTypeDef bme280_write8(BME280_Handle_t *dev, uint8_t reg, uint8_t value);
static HAL_StatusTypeDef bme280_read8(BME280_Handle_t *dev, uint8_t reg, uint8_t *value);
static HAL_StatusTypeDef bme280_read_block(BME280_Handle_t *dev, uint8_t reg, uint8_t *buf, uint16_t len);
static float bme280_compensate_temperature(BME280_Handle_t *dev, int32_t adc_T);
static float bme280_compensate_pressure(const BME280_Handle_t *dev, int32_t adc_P);
static float bme280_compensate_humidity(const BME280_Handle_t *dev, int32_t adc_H);

HAL_StatusTypeDef BME280_Init(BME280_Handle_t *dev)
{
  if (dev == NULL || dev->i2c == NULL)
  {
    return HAL_ERROR;
  }

  uint8_t id = 0U;
  HAL_StatusTypeDef status = bme280_read8(dev, BME280_REG_ID, &id);
  if (status != HAL_OK)
  {
    return status;
  }

  if (id != 0x60U)
  {
    return HAL_ERROR;
  }

  uint8_t reset = BME280_RESET_VALUE;
  status = bme280_write8(dev, BME280_REG_RESET, reset);
  if (status != HAL_OK)
  {
    return status;
  }

  HAL_Delay(2);

  status = bme280_read_calibration(dev);
  if (status != HAL_OK)
  {
    return status;
  }

  /* Configure for single forced measurements with minimal filtering */
  status = bme280_write8(dev, BME280_REG_CONFIG, 0x00U);
  if (status != HAL_OK)
  {
    return status;
  }

  /* Prime humidity oversampling; datasheet requires writing ctrl_hum before ctrl_meas */
  status = bme280_write8(dev, BME280_REG_CTRL_HUM, 0x01U);
  return status;
}

HAL_StatusTypeDef BME280_ReadMeasurements(BME280_Handle_t *dev, BME280_Measurement_t *out)
{
  if (dev == NULL || out == NULL)
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status = bme280_write8(dev, BME280_REG_CTRL_MEAS, 0x25U);
  if (status != HAL_OK)
  {
    return status;
  }

  uint8_t status_reg = 0U;
  uint32_t timeout = HAL_GetTick() + 20U;
  do
  {
    status = bme280_read8(dev, BME280_REG_STATUS, &status_reg);
    if (status != HAL_OK)
    {
      return status;
    }

    if (!(status_reg & BME280_STATUS_MEASURING))
    {
      break;
    }
  } while (HAL_GetTick() < timeout);

  if (status_reg & BME280_STATUS_MEASURING)
  {
    return HAL_TIMEOUT;
  }

  uint8_t raw[8] = {0};
  status = bme280_read_block(dev, BME280_REG_PRESS_MSB, raw, sizeof(raw));
  if (status != HAL_OK)
  {
    return status;
  }

  int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
  int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
  int32_t adc_H = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

  out->temperature_c = bme280_compensate_temperature(dev, adc_T);
  out->pressure_pa = bme280_compensate_pressure(dev, adc_P);
  out->humidity_rh = bme280_compensate_humidity(dev, adc_H);

  return HAL_OK;
}

static HAL_StatusTypeDef bme280_read_calibration(BME280_Handle_t *dev)
{
  uint8_t buf1[26];
  HAL_StatusTypeDef status = bme280_read_block(dev, 0x88U, buf1, sizeof(buf1));
  if (status != HAL_OK)
  {
    return status;
  }

  dev->calib.dig_T1 = (uint16_t)((buf1[1] << 8) | buf1[0]);
  dev->calib.dig_T2 = (int16_t)((buf1[3] << 8) | buf1[2]);
  dev->calib.dig_T3 = (int16_t)((buf1[5] << 8) | buf1[4]);
  dev->calib.dig_P1 = (uint16_t)((buf1[7] << 8) | buf1[6]);
  dev->calib.dig_P2 = (int16_t)((buf1[9] << 8) | buf1[8]);
  dev->calib.dig_P3 = (int16_t)((buf1[11] << 8) | buf1[10]);
  dev->calib.dig_P4 = (int16_t)((buf1[13] << 8) | buf1[12]);
  dev->calib.dig_P5 = (int16_t)((buf1[15] << 8) | buf1[14]);
  dev->calib.dig_P6 = (int16_t)((buf1[17] << 8) | buf1[16]);
  dev->calib.dig_P7 = (int16_t)((buf1[19] << 8) | buf1[18]);
  dev->calib.dig_P8 = (int16_t)((buf1[21] << 8) | buf1[20]);
  dev->calib.dig_P9 = (int16_t)((buf1[23] << 8) | buf1[22]);
  dev->calib.dig_H1 = buf1[25];

  uint8_t buf2[7];
  status = bme280_read_block(dev, 0xE1U, buf2, sizeof(buf2));
  if (status != HAL_OK)
  {
    return status;
  }

  dev->calib.dig_H2 = (int16_t)((buf2[1] << 8) | buf2[0]);
  dev->calib.dig_H3 = buf2[2];
  dev->calib.dig_H4 = (int16_t)((buf2[3] << 4) | (buf2[4] & 0x0FU));
  dev->calib.dig_H5 = (int16_t)((buf2[5] << 4) | (buf2[4] >> 4));
  dev->calib.dig_H6 = (int8_t)buf2[6];

  return HAL_OK;
}

static HAL_StatusTypeDef bme280_write8(BME280_Handle_t *dev, uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(dev->i2c, dev->address, reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, BME280_TIMEOUT_MS);
}

static HAL_StatusTypeDef bme280_read8(BME280_Handle_t *dev, uint8_t reg, uint8_t *value)
{
  return HAL_I2C_Mem_Read(dev->i2c, dev->address, reg, I2C_MEMADD_SIZE_8BIT, value, 1U, BME280_TIMEOUT_MS);
}

static HAL_StatusTypeDef bme280_read_block(BME280_Handle_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(dev->i2c, dev->address, reg, I2C_MEMADD_SIZE_8BIT, buf, len, BME280_TIMEOUT_MS);
}

static float bme280_compensate_temperature(BME280_Handle_t *dev, int32_t adc_T)
{
  if (adc_T == 0x800000)
  {
    return 0.0f;
  }

  float var1 = ((adc_T / 16384.0f) - (dev->calib.dig_T1 / 1024.0f)) * dev->calib.dig_T2;
  float var2 = (((adc_T / 131072.0f) - (dev->calib.dig_T1 / 8192.0f)) *
                ((adc_T / 131072.0f) - (dev->calib.dig_T1 / 8192.0f))) *
               dev->calib.dig_T3;

  dev->t_fine = (int32_t)(var1 + var2);
  return (var1 + var2) / 5120.0f;
}

static float bme280_compensate_pressure(const BME280_Handle_t *dev, int32_t adc_P)
{
  if (adc_P == 0x800000)
  {
    return 0.0f;
  }

  float var1 = (dev->t_fine / 2.0f) - 64000.0f;
  float var2 = var1 * var1 * (dev->calib.dig_P6 / 32768.0f);
  var2 = var2 + var1 * dev->calib.dig_P5 * 2.0f;
  var2 = (var2 / 4.0f) + (dev->calib.dig_P4 * 65536.0f);
  var1 = ((dev->calib.dig_P3 * var1 * var1 / 524288.0f) +
          (dev->calib.dig_P2 * var1)) /
             524288.0f;
  var1 = (1.0f + var1 / 32768.0f) * dev->calib.dig_P1;

  if (var1 == 0.0f)
  {
    return 0.0f;
  }

  float p = 1048576.0f - (float)adc_P;
  p = (p - (var2 / 4096.0f)) * 6250.0f / var1;
  var1 = dev->calib.dig_P9 * p * p / 2147483648.0f;
  var2 = p * dev->calib.dig_P8 / 32768.0f;
  p = p + (var1 + var2 + dev->calib.dig_P7) / 16.0f;

  return p;
}

static float bme280_compensate_humidity(const BME280_Handle_t *dev, int32_t adc_H)
{
  float var1 = dev->t_fine - 76800.0f;
  float var2 = (adc_H - (dev->calib.dig_H4 * 64.0f + dev->calib.dig_H5 / 16384.0f * var1)) *
               (dev->calib.dig_H2 / 65536.0f * (1.0f + dev->calib.dig_H6 / 67108864.0f * var1 * (1.0f + dev->calib.dig_H3 / 67108864.0f * var1)));
  float var3 = var2 * (1.0f - dev->calib.dig_H1 * var2 / 524288.0f);
  if (var3 > 100.0f)
  {
    var3 = 100.0f;
  }
  else if (var3 < 0.0f)
  {
    var3 = 0.0f;
  }
  return var3;
}


