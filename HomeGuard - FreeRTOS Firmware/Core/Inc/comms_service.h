
#ifndef INC_COMMS_SERVICE_H_
#define INC_COMMS_SERVICE_H_

#include "wifi_sensors.h"
#include "wifi_basic_driver.h"
#include "wifi_http_support.h"
#include "wifi_sensors.h"
#include "usart.h"
#include "stm32f4xx_hal.h"



#define WIFI_SSID     "Zer0"
#define WIFI_PASSWORD "Password"

void Comms_Init(void);
wifi_status_t Comms_Upload(const sensors_readings_t *data);

#endif
