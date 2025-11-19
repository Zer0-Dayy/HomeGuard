#ifndef ULTRASONIC_SENSOR_DRIVER_H
#define ULTRASONIC_SENSOR_DRIVER_H

#include "main.h"
#include <math.h>

typedef enum{
	ULTRASONIC_OK,
	ULTRASONIC_BUSY,
	ULTRASONIC_FAIL,
}ultrasonic_status_t;

typedef enum{
	WAITING_FOR_RISING,
	WAITING_FOR_FALLING,
}echo_status_t;

typedef struct{
	echo_status_t echo_state;
	uint8_t ultrasonic_ready ;
	TIM_HandleTypeDef *htim_echo;
	uint32_t echo_channel;
	TIM_HandleTypeDef *htim_trig;
	uint32_t trig_channel;
	uint16_t t_rise;
	uint16_t t_fall;
}ultrasonic_Handle_t;


void Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor);
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t* ultrasonic_sensor);
uint8_t Ultrasonic_IsReady(ultrasonic_Handle_t* ultrasonic_sensor);
float Ultrasonic_GetDistance(ultrasonic_Handle_t* ultrasonic_sensor);


#endif
