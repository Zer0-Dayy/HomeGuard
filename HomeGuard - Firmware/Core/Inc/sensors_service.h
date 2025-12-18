
#ifndef INC_SENSORS_SERVICE_H_
#define INC_SENSORS_SERVICE_H_

#include "wifi_sensors.h"
#include "bme280_sensor_driver.h"
#include "gaz_sensor_driver.h"
#include "i2c.h"
#include "adc.h"
#include "stm32f4xx_hal.h"

void Sensors_Init(void);
void Sensors_Read(sensors_readings_t *out);


#endif
