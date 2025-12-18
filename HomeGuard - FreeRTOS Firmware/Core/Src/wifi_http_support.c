#include "wifi_http_support.h"

wifi_status_t WiFi_HTTP_Send(const char* method, const char* host_ip, uint16_t port,const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries){
    
    if(method == NULL || method[0] == '\0' 
        || path == NULL || path[0] != '/' ||
         host_ip == NULL || host_ip[0] == '\0'){
        return WIFI_ERROR;
    }

    else{
        char request[512];
        memset(request,0,sizeof(request));
        uint16_t body_length = (body ? strlen(body) : 0);
        uint16_t offset = 0;
        uint16_t written = 0;
        written = snprintf(request, sizeof(request), "%s %s HTTP/1.1\r\n",method,path);
        if(written < 0 || written >= sizeof(request) - offset){
            return WIFI_ERROR;
        }
        offset += written;
        written = snprintf(request+offset, sizeof(request)-offset , "Host: %s\r\n", host_ip);
        if(written < 0 || written >= sizeof(request) - offset){
            return WIFI_ERROR;
        }
        offset += written;
        written = snprintf(request+offset, sizeof(request)-offset , "Connection: close\r\n");
        if(written < 0 || written >= sizeof(request) - offset){
            return WIFI_ERROR;
        }
        offset += written;
        if(body_length > 0){
           written = snprintf(request+offset, sizeof(request)-offset , "Content-Length: %u\r\n", body_length);
            if(written < 0 || written >= sizeof(request) - offset){
                return WIFI_ERROR;
            }
        offset += written;
        }
        if(headers != NULL){
            uint8_t has_content_type = 0;
            for(int i = 0; headers[i].key != NULL; i++){
                if(strcmp(headers[i].key,"Content-Type") == 0){
                    has_content_type = 1;
                }
                written = snprintf(request+offset, sizeof(request)-offset , "%s: %s\r\n", headers[i].key, headers[i].value);
                if(written < 0 || written >= sizeof(request) - offset){
                    return WIFI_ERROR;
                }
                offset += written;
            }
            if(has_content_type == 0){
                written = snprintf(request+offset, sizeof(request)-offset , "Content-Type: text/plain\r\n");
                if(written < 0 || written >= sizeof(request) - offset){
                    return WIFI_ERROR;
                }
            offset += written;    
            }
        }
        written = snprintf(request+offset, sizeof(request)-offset , "\r\n");
        if(written < 0 || written >= sizeof(request) - offset){
            return WIFI_ERROR;
        }
        offset += written;
        if(body_length > 0){
            if(body_length >= sizeof(request) - offset){
                return WIFI_ERROR; //NOT ENOUGH SPACE LEFT IN THE BUFFER.
            }
            memcpy(request+offset, body, body_length);
            offset += body_length;
        }
        for(int i = 1; i <= max_retries; i++){
            HAL_Delay(250);
            uint16_t total_length = offset;
            char cmd[128];
            snprintf(cmd,sizeof(cmd),"AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", host_ip, port);
            if(WiFi_Send_Command(cmd, "OK", 5000) != WIFI_OK){
                WIFI_LOG("Attempt %u --- WiFi_SendHTTP: CIPSTART failed!\r\n", i);
                WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", 2000);
                continue;
            }

            snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n",(unsigned int)total_length);
            if(WiFi_Send_Command(cmd,">",5000) != WIFI_OK){
                WIFI_LOG("Attempt %u --- WiFi_HTTP_Send: CIPSEND failed!\r\n", i);
                WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", 2000);
                continue;
            }
            
            if(WiFi_SendRaw((uint8_t*)request, total_length) != WIFI_OK){
                WIFI_LOG("Attempt %u --- WiFi_HTTP_Send: SendRaw failed!\r\n", i);
                WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", 2000);
                continue;
            }

            wifi_status_t status = WiFi_Expect("+IPD", timeout_ms);
            if (status != WIFI_OK){
                status = WiFi_Expect("CLOSED", timeout_ms);
            }
            if (status != WIFI_OK){
                WIFI_LOG("Attempt %u --- WiFi_HTTP_Send: No HTTP response detected\r\n", i);
                WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", 2000);
                continue;
            }
            else{
                WIFI_LOG("Attempt %u --- WiFi_HTTP_Send: HTTP response received\r\n", i);
            }
            WiFi_Send_Command("AT+CIPCLOSE\r\n", "OK", timeout_ms);
            return WIFI_OK;
        }
        return WIFI_ERROR;
    }
}

wifi_status_t WiFi_HTTP_POST(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries){
    return WiFi_HTTP_Send("POST", host_ip, port, path, headers, body, timeout_ms, max_retries);
}

wifi_status_t WiFi_HTTP_GET(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, uint32_t timeout_ms, uint8_t max_retries){
    return WiFi_HTTP_Send("GET", host_ip, port, path, headers, NULL, timeout_ms, max_retries);
}

wifi_status_t WiFi_HTTP_PUT(const char* host_ip, uint16_t port, const char* path, const wifi_http_header_t* headers, const char* body, uint32_t timeout_ms, uint8_t max_retries){
    return WiFi_HTTP_Send("PUT", host_ip, port, path, headers, body, timeout_ms, max_retries);
}

wifi_status_t WiFi_HTTP_DELETE(const char* host_ip, uint16_t port,const char* path, const wifi_http_header_t* headers, uint32_t timeout_ms, uint8_t max_retries){
    return WiFi_HTTP_Send("DELETE", host_ip, port, path, headers, NULL, timeout_ms, max_retries);
}
