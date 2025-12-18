#include "sensors_service.h"





static bme280_sensor_t bme280;
static gaz_sensor_t    mq2;

static uint8_t sensors_ready = 0;



void Sensors_Init(void)
{
    BME280_Sensor_Init(&bme280, &hi2c1, BME280_I2C_ADDR_LOW);

    if (BME280_Sensor_Begin(&bme280) != BME280_OK)
    {
        sensors_ready = 0;
        return;
    }


    Gaz_Sensor_Init(&mq2, &hadc1);


    HAL_Delay(1000);

    if (Gaz_Sensor_Calibrate(&mq2, 128, 50) != GAZ_SENSOR_OK)
    {
        sensors_ready = 0;
        return;
    }

    sensors_ready = 1;
}

void Sensors_Read(sensors_readings_t *out)
{
    if (!out || !sensors_ready)
    {
        return;
    }


    bme280_reading_t bme;

    if (BME280_Sensor_Read(&bme280, &bme) == BME280_OK)
    {
        out->temperature = bme.temperature_c;
        out->humidity    = bme.humidity_rh;
        out->pressure    = bme.pressure_pa / 100.0f;
    }


    gaz_sensor_reading_t gas;

    if (Gaz_Sensor_Read(&mq2, 32, &gas) == GAZ_SENSOR_OK)
    {
        out->gas = gas.ratio_vs_r0;
    }
}

