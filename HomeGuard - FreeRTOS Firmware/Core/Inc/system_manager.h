

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H



#include "sensors_service.h"
#include "comms_service.h"
#include "rtc_service.h"
#include "gpio.h"
#include "main.h"
#include "wifi_sensors.h"
#include "stm32f4xx_hal.h"
#include "movement_service.h"
#include "cmsis_os2.h"

#define MOVEMENT_CYCLE 60000U

typedef enum {
    SYSTEM_SLEEP = 0,
    SYSTEM_RUNNING = 1
} system_state_t;

typedef enum {
    MOVEMENT_PHASE = 0,
    DETECTION_PHASE = 1
} system_phase_t;

extern volatile system_state_t system_state;



void System_Init(void);
void System_RunOnce(void);
void System_Task(void *argument);

#endif
