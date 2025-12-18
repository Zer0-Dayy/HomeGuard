#include "comms_service.h"



void Comms_Init(void)
{

    WiFi_Init(&huart1);
    WiFi_Connect(WIFI_SSID, WIFI_PASSWORD);
}

wifi_status_t Comms_Upload(const sensors_readings_t *data)
{
    if (!data)
    {
        return WIFI_ERROR;
    }

    return WiFi_Sensors_Upload(data);
}
