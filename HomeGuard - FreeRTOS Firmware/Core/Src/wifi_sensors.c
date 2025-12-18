#include "wifi_sensors.h"

static int build_sensor_json(char* out, uint16_t max_len, const sensors_readings_t* data){
    if (!out || !data){
        return -1;
    } 
    int written = snprintf(out, max_len,
        "{\r\n"
        "  \"timestamp\": \"%s\",\r\n"
        "  \"temperature\": %.2f,\r\n"
        "  \"pressure\": %.2f,\r\n"
        "  \"humidity\": %.2f,\r\n"
        "  \"gas\": %.2f\r\n"
        "}",
        data->timestamp,
        data->temperature,
        data->pressure,
        data->humidity,
        data->gas
    );
    if (written < 0 || written >= max_len){
        return -1;
    }
    return written;
}

wifi_status_t WiFi_Sensors_Upload(const sensors_readings_t* data){
    wifi_http_header_t headers[] = {
        { "Content-Type", "application/json" },
        { NULL, NULL }
    };

    char json_buffer[256];
    int json_length = build_sensor_json(json_buffer, sizeof(json_buffer), data);
    if (json_length < 0) {
        WIFI_LOG("WiFi_Sensors_Upload: JSON build failed!\r\n");
        return WIFI_ERROR;
    }
    WIFI_LOG("WiFi_Sensors_Upload: JSON payload:\n%s\n", json_buffer);
    return WiFi_HTTP_POST(SENSOR_SERVER_IP,SENSOR_SERVER_PORT,SENSOR_SERVER_PATH,headers,json_buffer, TIMEOUT_MS, MAX_RETRIES);
}
