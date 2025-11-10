#ifndef WIFI_DRIVER_H
#define WIFI_DRIVER_H

#include "main.h"

#define WIFI_RX_BUF_LEN 128

typedef enum{
	WIFI_OK,
	WIFI_ERROR,
	WIFI_TIMEOUT,
	WIFI_BUSY
}wifi_status_t;

void WiFi_Init(UART_HandleTypeDef *huart);
wifi_status_t WiFi_Send_Command(char *cmd, const char *expected, uint32_t timeout_ms);
wifi_status_t WiFi_Connect(const char* ssid, const char* password);
wifi_status_t WiFi_GetIP(char* out_buf, uint16_t buf_len);
wifi_status_t WiFi_SendTCP(const char*ip, uint16_t port,char* message);

#endif
