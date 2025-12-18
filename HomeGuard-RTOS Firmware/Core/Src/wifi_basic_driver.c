#include "wifi_basic_driver.h"


static UART_HandleTypeDef *wifi_uart;
static uint8_t wifi_rx_buffer[WIFI_RX_BUF_LEN]; //DECLARING THE BUFFER TO READ DATA
static uint8_t wifi_rx_shadow_buffer[WIFI_RX_BUF_LEN]; //SECONDARY BUFFER TO READ DATA
volatile uint8_t wifi_rx_ready = 0; //RX STATE (TO ENSURE THE RX IS NOT BUSY READING DATA)
volatile uint16_t wifi_rx_size =0; //ACTUAL SIZE OF DATA BEING SENT FROM THE MODULE TO THE STM
volatile uint8_t wifi_tx_done = 1; //TX STATE (TO ENSURE THE TX IS NOT BUSY SENDING DATA)
volatile uint8_t wifi_response_ready = 0;

//THE INTERRUPT HANDLERS ARE WEAK FUNCTION BY DEFAULT, SO IT WOULD BE OPTIMAL TO REDEFINE THEM IN THE LIBRARY RATHER THAN THE MAIN PROGRAM.

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) //ONCE THE "HAL_UART_Transmit_IT()" FINISHED TRANSFERING BYTES, HAL DETECTS THE "TC" INTERRUPT AND CALLS THIS CALLBACK FUNCTION WHICH SETS "wifi_tx_done = 1" MEANING UART TX IS FREE.
{
    if (huart == wifi_uart)
        wifi_tx_done = 1; //SWITCH VARIABLE IF DATA TRANSFERED WAS SENT TO WIFI MODULE
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) //THE HAL INTERRUPT SERVICE ROUTINE SEES THE "IDLE" OR "BUFFER FULL" FLAG. IT CALLS THIS FUNCTION AND PASSES THE NUMBER OF BYTES RECEIVED. THIS FUNCTION NULL-TERMINATED THE STRING TO ENSURE SAFETY WHEN USING STRING FUNCTIONS.
{
    if (huart == wifi_uart)
    {
    	//COPY DATA TO SECONDARY BUFFER
    	memcpy(wifi_rx_shadow_buffer, wifi_rx_buffer, size);
    	wifi_rx_shadow_buffer[size] = '\0';

    	wifi_rx_size = size;
    	wifi_rx_ready = 1; //STM MODULE IS READY FOR RECEPTION AGAIN

    	memset(wifi_rx_buffer, 0, WIFI_RX_BUF_LEN); //CLEAR MEMORY BEFORE RE-ARMING THE RECEPTION FOR SAFETY.
        HAL_UARTEx_ReceiveToIdle_IT(wifi_uart,wifi_rx_buffer,WIFI_RX_BUF_LEN); //RECEPTION IS IMMEDIATELY ENABLED AGAIN AFTER SUCCESSFUL STRING PARSING.

    }
}

//INITIALIZE WI-FI MODULE WITH BASIC FUNCTIONS.
void WiFi_Init(UART_HandleTypeDef *huart){
	wifi_uart = huart;
	HAL_Delay(2000);
	HAL_UARTEx_ReceiveToIdle_IT(wifi_uart,wifi_rx_buffer,WIFI_RX_BUF_LEN);//START TRANSFERING BYTES INTO THE BUFFER. WHEN THE UART LINE STAYS HIGH FOR ONE FRAME TIME, IDLE FLAG IS TRIGGERED AND IT AUTOMATICALLY CALLS FOR "HAL_UARTEx_RxEventCallback()"
	//THIS PART CAN BE ENABLED FOR DEBUGGING
	WiFi_Send_Command("AT\r\n","OK",1000);
	WiFi_Send_Command("AT+CWMODE=1\r\n","OK",1000);
	printf("Wi-Fi module ready in STA mode. \r\n");
}

//SEND AN AT-COMMAND TO THE ESP8266 MODULE TO BE EXECUTED
wifi_status_t WiFi_Send_Command(char* Command, const char* expected, uint32_t timeout_ms){
	if(!wifi_tx_done){ //CHECK IF UART IS AVAILABLE TO RECEIVE COMMANDS
		return WIFI_BUSY;
	}
	wifi_tx_done = 0; //MARK UART AS BUSY UNTIL THE TRANSMISSION COMPLETE CALLBACK RUNS
	wifi_rx_ready = 0; //CLEAR READY FLAG BEFORE WAITING FOR A NEW RESPONSE
	memset(wifi_rx_shadow_buffer, 0, sizeof(wifi_rx_shadow_buffer));
    HAL_UART_Transmit_IT(wifi_uart, (uint8_t*)Command, (uint16_t)strlen(Command));//BASIC TRANSMITION FUNCTION THAT SEND THE COMMAND BIT BY BIT TO THE ESP8266 MODULE. TRIGGERS "TC" INTERRUPT WHEN COMPLETE.
    uint32_t tickstart = HAL_GetTick(); //INITIALIZE A COUNTDOWN
    while((HAL_GetTick() - tickstart) < timeout_ms){ //CHECK IF TIME SPENT WAITING FOR A RESPONSE IS LESS THAN USER DEFINED TIMEOUT
        if(wifi_rx_ready){ //CHECK IF STM IS READY TO RECEIVE DATA FROM WIFI MODULE
                if(strstr((char*)wifi_rx_shadow_buffer,expected)){ //CHECK IF THE DATA RECEIVED IS AS EXPECTED
                        wifi_rx_ready = 0;
                        return WIFI_OK;
                }
        }
        if(strstr((char*)wifi_rx_shadow_buffer,"ERROR")){ //CHECK IF THE MODULE THROWS AN ERROR
                return WIFI_ERROR;
        }
    }
    return WIFI_TIMEOUT; //RESPONSE TIMEOUT
}

//CONNECT DIRECTLY TO WIFI
wifi_status_t WiFi_Connect(const char *ssid, const char *password){
    char cmd[128]; //INITIALIZE A STRING THAT HOLDS THE AT-COMMAND FOR THE MODULE
    char ip[64];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password); //COPY THE COMMAND WITH APPROPRIATE SSID AND PASSWORD INTO THE "cmd" STRING
    if (WiFi_Send_Command(cmd, "OK", 15000) != WIFI_OK) {
    	return WIFI_ERROR;
    }

    memset(ip, 0, sizeof(ip));
    WiFi_GetIP(ip,sizeof(ip));
    if (ip[0] != '\0') {
    	WIFI_LOG("WiFi_Connect: connected, IP = %s\r\n", ip);
    	return WIFI_OK;
    } else {
    	WIFI_LOG("WiFi_Connect: no IP, connect failed\r\n");
    	return WIFI_ERROR;
    }
}

wifi_status_t WiFi_GetIP(char *out_buf, uint16_t buf_len){
	WiFi_Send_Command("AT+CIFSR\r\n","OK",2000); //RETURN THE IP ADDRESS OF THE STATION
	strncpy(out_buf,(char*)wifi_rx_shadow_buffer,buf_len); //COPY THE FULL TEXT INTO "out_buf"
	while(strstr((char*)wifi_rx_shadow_buffer, "busy p")) { //IF WIFI MODULE IS BUSY, WAIT 200ms AND TRY AGAIN
	    HAL_Delay(200);
	    return WiFi_Send_Command("AT+CIFSR\r\n","OK",2000);
	}
	return WIFI_OK;
}

wifi_status_t WiFi_SendTCP(const char*ip, uint16_t port, char* message){
	char cmd[128];
	//Opening TCP Connection
	snprintf(cmd,sizeof(cmd),"AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",ip,port); //AT+CIPSTART OPENS A TCP SOCKET
	wifi_status_t result = WiFi_Send_Command(cmd,"OK",5000); //TRY OPENING A TCP SOCKET
	if(result != WIFI_OK && !strstr((char*)wifi_rx_shadow_buffer,"CONNECT")){ //IF CONNECTION CANNOT BE ESTABLISHED POSSIBLE RETURN THE RESULTING SIGNAL
		return result;
	}

	snprintf(cmd,sizeof(cmd),"AT+CIPSEND=%u\r\n",(unsigned int)strlen(message)); //ALLOCATE SPACE IN THE WI-FI MODULE
	result = WiFi_Send_Command(cmd,">",2000);
	if(result != WIFI_OK){
		return result;
	}
	result = WiFi_Send_Command(message,"SEND OK",5000); //SEND PAYLOAD
	if(result != WIFI_OK){
		return result;
	}
	WiFi_Send_Command("AT+CIPCLOSE\r\n","OK",2000); //CLOSE CONNECTION
	return WIFI_OK;

}


wifi_status_t WiFi_SendRaw(const uint8_t* data, uint16_t length){
	while(wifi_tx_done == 0){
		HAL_Delay(1); //LOOP UNTIL TX LINE IS AVAILABLE
	}
	if(wifi_tx_done == 1){
		wifi_tx_done = 0; //ONCE TX LINE IS AVAILABLE: LOCK IT
		HAL_UART_Transmit_IT(wifi_uart,data, length); //TRANSMIT DATA TO ESP-01 MODULE
		while(wifi_tx_done == 0){
			HAL_Delay(1); //WAIT UNTIL DATA IS SENT
		}
	}
	return WIFI_OK;
}


wifi_status_t WiFi_Expect(const char *expected, uint32_t timeout_ms){
	wifi_rx_ready = 0;
	memset(wifi_rx_shadow_buffer,0,sizeof(wifi_rx_shadow_buffer));
	uint32_t start = HAL_GetTick();
	while(HAL_GetTick() - start < timeout_ms){

		if (wifi_rx_ready == 1){
			WIFI_LOG("WiFi_Expect: received -> %s\r\n", wifi_rx_shadow_buffer);
			if (strstr((char*)wifi_rx_shadow_buffer,expected)){
				memset(wifi_rx_shadow_buffer,0,sizeof(wifi_rx_shadow_buffer));
				wifi_rx_ready = 0;
				return WIFI_OK;
			}
			if (strstr((char*)wifi_rx_shadow_buffer,"ERROR")){
				memset(wifi_rx_shadow_buffer,0,sizeof(wifi_rx_shadow_buffer));
				wifi_rx_ready = 0;
				return WIFI_ERROR;
			}
			wifi_rx_ready = 0;
			memset(wifi_rx_shadow_buffer,0,sizeof(wifi_rx_shadow_buffer));
		}
	}
	WIFI_LOG("WiFi_Expect: TIMEOUT waiting for \"%s\"\r\n", expected);
	return WIFI_TIMEOUT;
}

const char* WiFi_StatusToString(wifi_status_t status){
	switch (status){
	case WIFI_OK:
		return "OK";
		case WIFI_ERROR:
			return "ERROR";
		case WIFI_TIMEOUT:
			return "TIMEOUT";
		case WIFI_BUSY:
			return "BUSY";
		default:
			return "UNKNOWN";
	}
}
