#include "wifi_driver.h"
#include <string.h>
#include <stdio.h>


static UART_HandleTypeDef *wifi_uart;
static uint8_t wifi_rx_buffer[WIFI_RX_BUF_LEN];
volatile uint8_t wifi_rx_ready = 0;
volatile uint16_t wifi_rx_size =0;
volatile uint8_t wifi_tx_done = 1;
volatile uint8_t wifi_response_ready = 0;


void WiFi_Init(UART_HandleTypeDef *huart){
	wifi_uart = huart;
	HAL_UARTEx_ReceiveToIdle_IT(wifi_uart,wifi_rx_buffer,WIFI_RX_BUF_LEN);
	HAL_Delay(1000);
	WiFi_Send_Command("AT\r\n","OK",1000);
	WiFi_Send_Command("AT+CWMODE=1\r\n","OK",1000);
	printf("Wi-Fi module ready in STA mode. \r\n");
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == wifi_uart)
        wifi_tx_done = 1;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart == wifi_uart)
    {
    	wifi_rx_size = size;
        wifi_rx_buffer[size] = '\0';
    	wifi_rx_ready = 1;
        HAL_UARTEx_ReceiveToIdle_IT(wifi_uart,wifi_rx_buffer,WIFI_RX_BUF_LEN);
        memset(wifi_rx_buffer, 0, WIFI_RX_BUF_LEN);
    }
}

wifi_status_t WiFi_Send_Command(char* Command, const char* expected, uint32_t timeout_ms)
{
	if(!wifi_tx_done){
		return WIFI_BUSY;
	}
    HAL_UART_Transmit_IT(wifi_uart, (uint8_t*)Command, (uint16_t)strlen(Command));
    uint32_t tickstart = HAL_GetTick();
    while((HAL_GetTick() - tickstart) < timeout_ms){
    	if(wifi_rx_ready){
    		if(strstr((char*)wifi_rx_buffer,expected)){
    			return WIFI_OK;
    		}
    	}
    	if(strstr((char*)wifi_rx_buffer,"ERROR")){
    		return WIFI_ERROR;
    	}
    }
    return WIFI_TIMEOUT;
}

wifi_status_t WiFi_Connect(const char *ssid, const char *password)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    return WiFi_Send_Command(cmd, "WIFI GOT IP", 10000);
}

wifi_status_t WiFi_GetIP(char *out_buf, uint16_t buf_len){
	WiFi_Send_Command("AT+CIFSR\r\n","OK",2000);
	strncpy(out_buf,(char*)wifi_rx_buffer,buf_len);
	while(strstr((char*)wifi_rx_buffer, "busy p")) {
	    HAL_Delay(200);
	    return WiFi_Send_Command("AT+CIFSR\r\n","OK",2000);
	}
	return WIFI_OK;
}

wifi_status_t WiFi_SendTCP(const char*ip, uint16_t port, char* message){
	char cmd[128];
	//Opening TCP Connection
	snprintf(cmd,sizeof(cmd),"AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",ip,port);
	wifi_status_t result = WiFi_Send_Command(cmd,"OK",5000);
	if(result != WIFI_OK && !strstr((char*)wifi_rx_buffer,"CONNECT")){
		return result;
	}
	//Allocate space in the ESP
	snprintf(cmd,sizeof(cmd),"AT+CIPSEND=%u\r\n",(unsigned int)strlen(message));
	result = WiFi_Send_Command(cmd,">",2000);
	if(result != WIFI_OK){
		return result;
	}
	//Send Actual Payload
	result = WiFi_Send_Command(message,"SEND OK",5000);
	if(result != WIFI_OK){
		return result;
	}
	//Close connection
	WiFi_Send_Command("AT+CIPCLOSE\r\n","OK",2000);
	return WIFI_OK;

}


