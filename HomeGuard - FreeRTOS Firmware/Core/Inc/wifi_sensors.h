#ifndef WIFI_SENSORS_H
#define WIFI_SENSORS_H

#include "wifi_http_support.h"

#define SENSOR_SERVER_IP   "192.168.26.119"
#define SENSOR_SERVER_PORT 5000
#define SENSOR_SERVER_PATH "/upload"
#define MAX_RETRIES 5
#define TIMEOUT_MS 5000

typedef struct {
    char timestamp[32];
    float temperature;
    float pressure;
    float humidity;
    float gas;
} sensors_readings_t;


wifi_status_t WiFi_Sensors_Upload(const sensors_readings_t* data);

#endif
