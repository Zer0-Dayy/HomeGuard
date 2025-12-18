#include "gaz_sensor_driver.h"

// Blocking, polling-based MQ-2 helper; keeps things predictable in a simple main loop.

// ADC is configured for 12-bit conversions, so 4095 equals full-scale.( 2 power 12 = 4095)
#define GAZ_SENSOR_ADC_FULL_SCALE 4095.0f

static HAL_StatusTypeDef Gaz_Sensor_ReadRawAverage(const gaz_sensor_t *sensor, uint32_t sample_count, uint16_t *average_out);
static float Gaz_Sensor_ComputeVoltage(const gaz_sensor_t *sensor, uint16_t adc_raw);
static float Gaz_Sensor_ComputeResistance(const gaz_sensor_t *sensor, uint16_t adc_raw);

// Initialize the driver context with defaults and bind the HAL ADC handle.
void Gaz_Sensor_Init(gaz_sensor_t *sensor, ADC_HandleTypeDef *hadc)
{
    if (sensor == NULL)
    {
        return;
    }

    sensor->hadc = hadc;
    sensor->vref_volts = 3.3f;            // Matches typical STM32 ADC Vref+
    sensor->load_resistance_ohms = 5000.0f; // RL on the board (5k)
    sensor->r0_ohms = 10000.0f;           // Placeholder until calibration writes the real value
    sensor->baseline_ready = 0U;          // Force callers to calibrate before trusting ratios
}

// Calibrate in clean air to determine R0. Averaging smooths noise; optional
// settle_ms lets the heater stabilize between samples when needed.
gaz_sensor_status_t Gaz_Sensor_Calibrate(gaz_sensor_t *sensor, uint32_t sample_count, uint32_t settle_ms)
{
    if (sensor == NULL || sample_count == 0U)
    {
        return GAZ_SENSOR_ERROR;
    }

    uint32_t accumulator = 0U;

    // Build an integer average of single-sample reads; optional delay gives the heater and ADC time to settle.
    for (uint32_t i = 0; i < sample_count; ++i)
    {
        uint16_t raw = 0U;
        if (Gaz_Sensor_ReadRawAverage(sensor, 1U, &raw) != HAL_OK)
        {
            sensor->baseline_ready = 0U;
            return GAZ_SENSOR_ERROR;
        }

        accumulator += raw;

        if (settle_ms > 0U)
        {
            HAL_Delay(settle_ms);
        }
    }

    uint16_t average_raw = (uint16_t)(accumulator / sample_count);
    sensor->r0_ohms = Gaz_Sensor_ComputeResistance(sensor, average_raw); // Clean-air Rs becomes R0 reference
    sensor->baseline_ready = (isfinite(sensor->r0_ohms) && sensor->r0_ohms > 0.0f) ? 1U : 0U;

    return sensor->baseline_ready ? GAZ_SENSOR_OK : GAZ_SENSOR_ERROR;
}

// Take N samples, average them, and report voltage, Rs, and Rs/R0 (if ready).
gaz_sensor_status_t Gaz_Sensor_Read(gaz_sensor_t *sensor, uint32_t sample_count, gaz_sensor_reading_t *reading)
{
    if (sensor == NULL || reading == NULL || sample_count == 0U)
    {
        return GAZ_SENSOR_ERROR;
    }

    uint16_t adc_raw = 0U;
    // Average a handful of conversions to knock down random ADC noise.
    if (Gaz_Sensor_ReadRawAverage(sensor, sample_count, &adc_raw) != HAL_OK)
    {
        return GAZ_SENSOR_ERROR;
    }

    reading->raw_counts = adc_raw;
    reading->voltage_volts = Gaz_Sensor_ComputeVoltage(sensor, adc_raw);
    reading->resistance_ohms = Gaz_Sensor_ComputeResistance(sensor, adc_raw);

    if (sensor->baseline_ready && sensor->r0_ohms > 0.0f && isfinite(sensor->r0_ohms))
    {
        reading->ratio_vs_r0 = reading->resistance_ohms / sensor->r0_ohms;
    }
    else
    {
        reading->ratio_vs_r0 = NAN; // Leave obvious placeholder when calibration hasn't happened
    }

    return GAZ_SENSOR_OK;
}

// Blocking helper that averages synchronous ADC conversions for noise reduction.
static HAL_StatusTypeDef Gaz_Sensor_ReadRawAverage(const gaz_sensor_t *sensor, uint32_t sample_count, uint16_t *average_out)
{
    if (sensor == NULL || sensor->hadc == NULL || sample_count == 0U || average_out == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t accumulator = 0U;

    // Polling each conversion keeps the code simple; we stop after each to avoid stale data.
    for (uint32_t i = 0U; i < sample_count; ++i)
    {
        HAL_StatusTypeDef status = HAL_ADC_Start(sensor->hadc);
        if (status != HAL_OK)
        {
            return status;
        }

        status = HAL_ADC_PollForConversion(sensor->hadc, HAL_MAX_DELAY);
        if (status != HAL_OK)
        {
            HAL_ADC_Stop(sensor->hadc);
            return status;
        }

        accumulator += HAL_ADC_GetValue(sensor->hadc);
        HAL_ADC_Stop(sensor->hadc);
    }

    *average_out = (uint16_t)(accumulator / sample_count);
    return HAL_OK;
}

// Convert ADC code to input voltage using configured vref.
static float Gaz_Sensor_ComputeVoltage(const gaz_sensor_t *sensor, uint16_t adc_raw)
{
    if (sensor == NULL || adc_raw > (uint16_t)GAZ_SENSOR_ADC_FULL_SCALE)
    {
        return NAN; // Caller can treat this as an obvious failure case
    }

    return (adc_raw / GAZ_SENSOR_ADC_FULL_SCALE) * sensor->vref_volts;
}

// Compute Rs from the divider equation using the known load resistor.
static float Gaz_Sensor_ComputeResistance(const gaz_sensor_t *sensor, uint16_t adc_raw)
{
    if (sensor == NULL)
    {
        return NAN;
    }

    if (adc_raw == 0U)
    {
        return INFINITY; // Open circuit: sensor is effectively infinite resistance
    }

    if (adc_raw >= (uint16_t)GAZ_SENSOR_ADC_FULL_SCALE)
    {
        return 0.0f; // Short to ground or sensor driven to zero ohms
    }

    float voltage = Gaz_Sensor_ComputeVoltage(sensor, adc_raw);
    return (sensor->load_resistance_ohms * (sensor->vref_volts - voltage)) / voltage;
}
