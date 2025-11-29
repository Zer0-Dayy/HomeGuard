#ifndef WIFI_HTTP_SUPPORT_H
#define WIFI_HTTP_SUPPORT_H

#include "wifi_basic_driver.h"

typedef struct{
    const char* key;
    const char* value;
}wifi_http_header_t;

wifi_status_t WiFi_HTTP_Send(const char* method, const char* host_ip, uint16_t port,const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries);
wifi_status_t WiFi_HTTP_POST(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries);
wifi_status_t WiFi_HTTP_GET(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, uint32_t timeout_ms, uint8_t max_retries);
wifi_status_t WiFi_HTTP_PUT(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries);
wifi_status_t WiFi_HTTP_DELETE(const char* host_ip, uint16_t port,const char* path, const wifi_http_header_t* headers, uint32_t timeout_ms, uint8_t max_retries);


#endif