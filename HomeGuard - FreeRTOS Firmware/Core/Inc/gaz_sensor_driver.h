#ifndef GAZ_SENSOR_DRIVER_H
#define GAZ_SENSOR_DRIVER_H

/**
 * Minimal blocking driver for the MQ-2 combustible gas sensor.
 *
 * The ADC is polled synchronously, averaged in software, converted to voltage,
 * and then to sensor resistance using a known load resistor. A clean-air
 * calibration step computes R0 so callers can read Rs/R0 directly.
 *
 * Typical use:
 *   gaz_sensor_t sensor;
 *   Gaz_Sensor_Init(&sensor, &hadc1);
 *   Gaz_Sensor_Calibrate(&sensor, 128, 50); // in clean air
 *   Gaz_Sensor_Read(&sensor, 32, &reading); // periodic measurements
 */

#include "main.h"
#include <math.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAZ_SENSOR_OK = 0,
    GAZ_SENSOR_ERROR
} gaz_sensor_status_t;

// Raw ADC plus derived electrical values.
typedef struct {
    uint16_t raw_counts;        // Averaged ADC code
    float voltage_volts;        // ADC code mapped to vref
    float resistance_ohms;      // Sensor resistance Rs
    float ratio_vs_r0;          // Rs/R0 (NAN if not calibrated)
} gaz_sensor_reading_t;

// Driver context and calibration state.
typedef struct {
    ADC_HandleTypeDef *hadc;    // HAL ADC handle
    float vref_volts;           // ADC reference voltage (should mirror CubeMX ADC config)
    float load_resistance_ohms; // Actual RL on the PCB; used to back-calc sensor resistance
    float r0_ohms;              // Saved clean-air resistance reference (computed once)
    uint8_t baseline_ready;     // Flag so callers know if ratio_vs_r0 is trustworthy
} gaz_sensor_t;

// Bind the ADC handle and electrical defaults.
void Gaz_Sensor_Init(gaz_sensor_t *sensor, ADC_HandleTypeDef *hadc);
// Sample clean air to compute R0; call once after warm-up.
gaz_sensor_status_t Gaz_Sensor_Calibrate(gaz_sensor_t *sensor, uint32_t sample_count, uint32_t settle_ms);
// Read averaged ADC and fill voltage, Rs, and Rs/R0 (when calibrated).
gaz_sensor_status_t Gaz_Sensor_Read(gaz_sensor_t *sensor, uint32_t sample_count, gaz_sensor_reading_t *reading);

#ifdef __cplusplus
}
#endif

#endif /* GAZ_SENSOR_DRIVER_H */
