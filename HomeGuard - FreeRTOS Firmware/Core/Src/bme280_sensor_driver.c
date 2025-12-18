#include "bme280_sensor_driver.h"
#include <string.h>

// Register addresses from the BME280 datasheet
#define BME280_REG_ID        0xD0U
#define BME280_REG_RESET     0xE0U
#define BME280_REG_CTRL_HUM  0xF2U
#define BME280_REG_STATUS    0xF3U
#define BME280_REG_CTRL_MEAS 0xF4U
#define BME280_REG_CONFIG    0xF5U
#define BME280_REG_PRESS_MSB 0xF7U

#define BME280_CHIP_ID       0x60U
#define BME280_RESET_VALUE   0xB6U
#define BME280_STATUS_MEASURING (1U << 3)


#define BME280_CTRL_HUM_VAL  0x01U                // Humidity oversampling x1
#define BME280_CTRL_MEAS_FORCED (0x20U | 0x04U | 0x01U) // Temp x1 (bits 7:5), Press x1 (4:2), forced (1:0)
#define BME280_CONFIG_VAL    0x00U                // No filter, 0.5 ms standby; simplest path for labs

static bme280_status_t bme280_read_calibration(bme280_sensor_t *sensor);
static bme280_status_t bme280_write8(bme280_sensor_t *sensor, uint8_t reg, uint8_t value);
static bme280_status_t bme280_read8(bme280_sensor_t *sensor, uint8_t reg, uint8_t *value);
static bme280_status_t bme280_read_block(bme280_sensor_t *sensor, uint8_t reg, uint8_t *buf, uint16_t len);
static float bme280_compensate_temperature(bme280_sensor_t *sensor, int32_t adc_T);
static float bme280_compensate_pressure(const bme280_sensor_t *sensor, int32_t adc_P);
static float bme280_compensate_humidity(const bme280_sensor_t *sensor, int32_t adc_H);

void BME280_Sensor_Init(bme280_sensor_t *sensor, I2C_HandleTypeDef *hi2c, uint8_t i2c_address)
{
    if (sensor == NULL)
    {
        return;
    }

    memset(sensor, 0, sizeof(*sensor));
    sensor->hi2c = hi2c;
    sensor->i2c_address = i2c_address;
}

bme280_status_t BME280_Sensor_Begin(bme280_sensor_t *sensor)
{
    if (sensor == NULL || sensor->hi2c == NULL)
    {
        return BME280_ERROR;
    }

    uint8_t id = 0U;
    if (bme280_read8(sensor, BME280_REG_ID, &id) != BME280_OK)
    {
        return BME280_ERROR;
    }

    if (id != BME280_CHIP_ID)
    {
        return BME280_ERROR;
    }

    // Soft reset to ensure a known state
    if (bme280_write8(sensor, BME280_REG_RESET, BME280_RESET_VALUE) != BME280_OK)
    {
        return BME280_ERROR;
    }
    HAL_Delay(2); // Datasheet: typical reset time ~2 ms

    if (bme280_read_calibration(sensor) != BME280_OK)
    {
        return BME280_ERROR;
    }

    // Configure filters/standby (off), humidity oversampling (must be written before ctrl_meas)
    if (bme280_write8(sensor, BME280_REG_CONFIG, BME280_CONFIG_VAL) != BME280_OK)
    {
        return BME280_ERROR;
    }
    if (bme280_write8(sensor, BME280_REG_CTRL_HUM, BME280_CTRL_HUM_VAL) != BME280_OK)
    {
        return BME280_ERROR;
    }

    return BME280_OK;
}

bme280_status_t BME280_Sensor_Read(bme280_sensor_t *sensor, bme280_reading_t *reading)
{
    if (sensor == NULL || reading == NULL)
    {
        return BME280_ERROR;
    }

    // Kick off one forced conversion using the chosen oversampling.
    if (bme280_write8(sensor, BME280_REG_CTRL_MEAS, BME280_CTRL_MEAS_FORCED) != BME280_OK)
    {
        return BME280_ERROR;
    }

    // Poll the status bit; conversion time at x1 oversampling is ~7 ms.
    uint32_t timeout = HAL_GetTick() + 20U; // 20 ms guard
    uint8_t status = 0U;
    do
    {
        if (bme280_read8(sensor, BME280_REG_STATUS, &status) != BME280_OK)
        {
            return BME280_ERROR;
        }
        if ((status & BME280_STATUS_MEASURING) == 0U)
        {
            break;
        }
    } while (HAL_GetTick() < timeout);

    if (status & BME280_STATUS_MEASURING)
    {
        return BME280_TIMEOUT;
    }

    // Read pressure (3 bytes), temperature (3 bytes), humidity (2 bytes) in one burst
    uint8_t raw[8] = {0};
    if (bme280_read_block(sensor, BME280_REG_PRESS_MSB, raw, sizeof(raw)) != BME280_OK)
    {
        return BME280_ERROR;
    }

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8)  | ((int32_t)raw[7]);

    reading->temperature_c = bme280_compensate_temperature(sensor, adc_T);
    reading->pressure_pa   = bme280_compensate_pressure(sensor, adc_P);
    reading->humidity_rh   = bme280_compensate_humidity(sensor, adc_H);

    return BME280_OK;
}

// --- Low-level helpers ---
static bme280_status_t bme280_read_calibration(bme280_sensor_t *sensor)
{
    uint8_t buf1[26];
    if (bme280_read_block(sensor, 0x88U, buf1, sizeof(buf1)) != BME280_OK)
    {
        return BME280_ERROR;
    }

    sensor->calib.dig_T1 = (uint16_t)((buf1[1] << 8) | buf1[0]);
    sensor->calib.dig_T2 = (int16_t)((buf1[3] << 8) | buf1[2]);
    sensor->calib.dig_T3 = (int16_t)((buf1[5] << 8) | buf1[4]);
    sensor->calib.dig_P1 = (uint16_t)((buf1[7] << 8) | buf1[6]);
    sensor->calib.dig_P2 = (int16_t)((buf1[9] << 8) | buf1[8]);
    sensor->calib.dig_P3 = (int16_t)((buf1[11] << 8) | buf1[10]);
    sensor->calib.dig_P4 = (int16_t)((buf1[13] << 8) | buf1[12]);
    sensor->calib.dig_P5 = (int16_t)((buf1[15] << 8) | buf1[14]);
    sensor->calib.dig_P6 = (int16_t)((buf1[17] << 8) | buf1[16]);
    sensor->calib.dig_P7 = (int16_t)((buf1[19] << 8) | buf1[18]);
    sensor->calib.dig_P8 = (int16_t)((buf1[21] << 8) | buf1[20]);
    sensor->calib.dig_P9 = (int16_t)((buf1[23] << 8) | buf1[22]);
    sensor->calib.dig_H1 = buf1[25];

    uint8_t buf2[7];
    if (bme280_read_block(sensor, 0xE1U, buf2, sizeof(buf2)) != BME280_OK)
    {
        return BME280_ERROR;
    }

    sensor->calib.dig_H2 = (int16_t)((buf2[1] << 8) | buf2[0]);
    sensor->calib.dig_H3 = buf2[2];
    sensor->calib.dig_H4 = (int16_t)((buf2[3] << 4) | (buf2[4] & 0x0FU));
    sensor->calib.dig_H5 = (int16_t)((buf2[5] << 4) | (buf2[4] >> 4));
    sensor->calib.dig_H6 = (int8_t)buf2[6];

    return BME280_OK;
}

static bme280_status_t bme280_write8(bme280_sensor_t *sensor, uint8_t reg, uint8_t value)
{
    return (HAL_I2C_Mem_Write(sensor->hi2c, sensor->i2c_address, reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, HAL_MAX_DELAY) == HAL_OK)
               ? BME280_OK
               : BME280_ERROR;
}

static bme280_status_t bme280_read8(bme280_sensor_t *sensor, uint8_t reg, uint8_t *value)
{
    return (HAL_I2C_Mem_Read(sensor->hi2c, sensor->i2c_address, reg, I2C_MEMADD_SIZE_8BIT, value, 1U, HAL_MAX_DELAY) == HAL_OK)
               ? BME280_OK
               : BME280_ERROR;
}

static bme280_status_t bme280_read_block(bme280_sensor_t *sensor, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(sensor->hi2c, sensor->i2c_address, reg, I2C_MEMADD_SIZE_8BIT, buf, len, HAL_MAX_DELAY) == HAL_OK)
               ? BME280_OK
               : BME280_ERROR;
}

// --- Compensation formulas from datasheet section 4.2.3 ---
static float bme280_compensate_temperature(bme280_sensor_t *sensor, int32_t adc_T)
{
    if (adc_T == 0x800000)
    {
        return 0.0f;
    }

    float var1 = ((adc_T / 16384.0f) - (sensor->calib.dig_T1 / 1024.0f)) * sensor->calib.dig_T2;
    float var2 = (((adc_T / 131072.0f) - (sensor->calib.dig_T1 / 8192.0f)) *
                  ((adc_T / 131072.0f) - (sensor->calib.dig_T1 / 8192.0f))) *
                 sensor->calib.dig_T3;

    sensor->t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0f;
}

static float bme280_compensate_pressure(const bme280_sensor_t *sensor, int32_t adc_P)
{
    if (adc_P == 0x800000)
    {
        return 0.0f;
    }

    float var1 = (sensor->t_fine / 2.0f) - 64000.0f;
    float var2 = var1 * var1 * (sensor->calib.dig_P6 / 32768.0f);
    var2 = var2 + (var1 * sensor->calib.dig_P5 * 2.0f);
    var2 = (var2 / 4.0f) + (sensor->calib.dig_P4 * 65536.0f);
    var1 = ((sensor->calib.dig_P3 * var1 * var1 / 524288.0f) + (sensor->calib.dig_P2 * var1)) / 524288.0f;
    var1 = (1.0f + (var1 / 32768.0f)) * sensor->calib.dig_P1;

    if (var1 == 0.0f)
    {
        return 0.0f; // Avoid divide by zero
    }

    float pressure = 1048576.0f - (float)adc_P;
    pressure = (pressure - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = sensor->calib.dig_P9 * pressure * pressure / 2147483648.0f;
    var2 = pressure * sensor->calib.dig_P8 / 32768.0f;
    pressure = pressure + ((var1 + var2 + sensor->calib.dig_P7) / 16.0f);

    return pressure;
}

static float bme280_compensate_humidity(const bme280_sensor_t *sensor, int32_t adc_H)
{
    float var1 = ((float)sensor->t_fine) - 76800.0f;
    float var2 = (sensor->calib.dig_H4 * 64.0f) + ((sensor->calib.dig_H5 / 16384.0f) * var1);
    float var3 = adc_H - var2;
    float var4 = sensor->calib.dig_H2 / 65536.0f;
    float var5 = (1.0f + (sensor->calib.dig_H3 / 67108864.0f) * var1);
    float var6 = 1.0f + (sensor->calib.dig_H6 / 67108864.0f) * var1 * var5;
    float humidity = var3 * var4 * (var5 * var6);

    // Clamp to sensible range
    if (humidity > 100.0f)
    {
        humidity = 100.0f;
    }
    else if (humidity < 0.0f)
    {
        humidity = 0.0f;
    }

    return humidity;
}
