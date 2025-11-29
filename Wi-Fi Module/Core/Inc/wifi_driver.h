#ifndef WIFI_DRIVER_H
#define WIFI_DRIVER_H

#include "main.h"
#include <string.h>
#include <stdio.h>

#define WIFI_RX_BUF_LEN 128

typedef enum{
	WIFI_OK,
	WIFI_ERROR,
	WIFI_TIMEOUT,
	WIFI_BUSY

}wifi_status_t;

typedef struct{
	char timestamp[32];
	float temperature;
	float pressure;
	float humidity;
	float gas;
}sensors_readings_t;

void WiFi_Init(UART_HandleTypeDef *huart);
wifi_status_t WiFi_Send_Command(char *cmd, const char *expected, uint32_t timeout_ms);
wifi_status_t WiFi_Connect(const char* ssid, const char* password);
wifi_status_t WiFi_GetIP(char* out_buf, uint16_t buf_len);
wifi_status_t WiFi_SendTCP(const char*ip, uint16_t port,char* message);
int build_json(char* output, uint16_t output_size, const sensors_readings_t *data);
int build_http_request(char* output, uint16_t output_size, uint16_t body_length);
wifi_status_t WiFi_SendHTTP(const char* ip, uint16_t port, const sensors_readings_t *data);
wifi_status_t WiFi_SendRaw(const uint8_t* data, uint16_t len);
wifi_status_t WiFi_Expect(const char *expected, uint32_t timeout_ms);
const char* WiFi_StatusToString(wifi_status_t status);
wifi_status_t WiFi_SendSensorReadings(const char* ip, uint16_t port, const sensors_readings_t *data, uint8_t max_retries);
#endif
