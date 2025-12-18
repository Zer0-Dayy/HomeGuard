
#ifndef MOVEMENT_SERVICE_H_
#define MOVEMENT_SERVICE_H_

#include "tim.h"
#include "Stepper_UltrasonicSensor.h"
#include <stdio.h>


void Movement_Init(void);
void Movement_Update(void);
void Movement_ReInitAfterStop(void);
void Movement_HandleTimerIRQ(TIM_HandleTypeDef *htim);


#endif
