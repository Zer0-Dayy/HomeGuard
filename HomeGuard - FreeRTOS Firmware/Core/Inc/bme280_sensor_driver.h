#ifndef BME280_SENSOR_DRIVER_H
#define BME280_SENSOR_DRIVER_H
/**
 * Blocking, classroom-friendly driver for the Bosch BME280 (temperature / humidity / pressure).
 *
 * Goals:
 *   - Minimal state stored in one struct that the user fills and passes.
 *   - Forced-mode measurements: we trigger one conversion, poll for completion, then read results.
 *   - Simple math: datasheet compensation formulas implemented with floats for clarity.
 *
 * Typical use:
 *   bme280_sensor_t sensor;
 *   BME280_Sensor_Init(&sensor, &hi2c1, BME280_I2C_ADDR_LOW); // PB8/PB9, address 0x76
 *   if (BME280_Sensor_Begin(&sensor) == BME280_OK) {
 *       bme280_reading_t sample;
 *       BME280_Sensor_Read(&sensor, &sample);
 *   }
 */

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The breakout typically uses SDO pulled low -> 0x76. If SDO high, use HIGH.
#define BME280_I2C_ADDR_LOW  (0x76U << 1) // Shifted for HAL 8-bit addressing
#define BME280_I2C_ADDR_HIGH (0x77U << 1)

typedef enum
{
    BME280_OK = 0,
    BME280_ERROR,
    BME280_TIMEOUT
} bme280_status_t;


// Calibration constants 
// The sensor stores these coefficients so your driver can use them to convert raw temperature/pressure/humidity readings into real-world values.
// They’re read off the chip itself. 
// During BME280_Sensor_Begin the driver calls bme280_read_calibration (in Temperature-Sensor-Updated/BME-280/Core/Src/bme280_sensor_driver.c), which reads the factory-trimmed calibration registers over I²C: block 0x88–0xA1 for temp/pressure and 0xE1–0xE7 for humidity. 
// Those registers are in the Bosch datasheet.
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
} bme280_calibration_t;

// Driver context stored by the application.
typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_address;
    bme280_calibration_t calib;
    int32_t t_fine; // Shared temp compensation term reused by pressure/humidity
} bme280_sensor_t;

typedef struct
{
    float temperature_c; // Degrees Celsius
    float humidity_rh;   // % relative humidity (0-100)
    float pressure_pa;   // Pascals
} bme280_reading_t;

// Bind the HAL I2C handle and I2C address (0x76 or 0x77). Does not talk to hardware yet.
void BME280_Sensor_Init(bme280_sensor_t *sensor, I2C_HandleTypeDef *hi2c, uint8_t i2c_address);

// Probe the sensor (checks ID), soft-resets it, reads calibration, and configures oversampling.
bme280_status_t BME280_Sensor_Begin(bme280_sensor_t *sensor);

// Trigger one forced measurement and fill the reading struct with compensated values.
bme280_status_t BME280_Sensor_Read(bme280_sensor_t *sensor, bme280_reading_t *reading);

#ifdef __cplusplus
}
#endif

#endif /* BME280_SENSOR_DRIVER_H */
