#ifndef STEPPER_ULTRASONICSENSOR_H
#define STEPPER_ULTRASONICSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic HAL / std types */
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <math.h>
#include "main.h"
/* ========================================================================== */
/*                         1) Stepper28BYJ Driver                             */
/* ========================================================================== */

#define STEPPER28BYJ_STEPS_PER_REV (2048U)
#define STEPPER28BYJ_DIR_FORWARD   (+1)
#define STEPPER28BYJ_DIR_REVERSE   (-1)

typedef enum
{
    STEPPER28BYJ_MOTOR_A = 0,
    STEPPER28BYJ_MOTOR_B = 1,
    STEPPER28BYJ_MOTOR_MAX
} Stepper28BYJ_Motor;

typedef enum
{
    STEPPER28BYJ_SPEED_LOW,
    STEPPER28BYJ_SPEED_MEDIUM,
    STEPPER28BYJ_SPEED_HIGH
} Stepper28BYJ_Speed;

/* Low-level stepper control functions */
HAL_StatusTypeDef Stepper28BYJ_Init(Stepper28BYJ_Motor motor,
                                    TIM_HandleTypeDef *timer,
                                    GPIO_TypeDef *portCoil1, uint16_t pinCoil1,
                                    GPIO_TypeDef *portCoil2, uint16_t pinCoil2,
                                    GPIO_TypeDef *portCoil3, uint16_t pinCoil3,
                                    GPIO_TypeDef *portCoil4, uint16_t pinCoil4);

HAL_StatusTypeDef Stepper28BYJ_Move(Stepper28BYJ_Motor motor, uint16_t steps, int8_t direction);
HAL_StatusTypeDef Stepper28BYJ_Stop(Stepper28BYJ_Motor motor);
uint8_t          Stepper28BYJ_IsBusy(Stepper28BYJ_Motor motor);
HAL_StatusTypeDef Stepper28BYJ_SetSpeedPreset(Stepper28BYJ_Speed preset);
void Stepper28BYJ_HandleTimerInterrupt(TIM_HandleTypeDef *htim);

/* ========================================================================== */
/*                    2) Ultrasonic Sensor Driver (HC-SR04)                   */
/* ========================================================================== */

#define ULTRASONIC_MAX_SENSORS 4

typedef enum{
    ULTRASONIC_OK,
    ULTRASONIC_BUSY,
    ULTRASONIC_FAIL,
} ultrasonic_status_t;

typedef enum{
    WAITING_FOR_RISING,
    WAITING_FOR_FALLING,
} echo_status_t;

typedef struct{
    echo_status_t echo_state;
    uint8_t ultrasonic_ready;
    TIM_HandleTypeDef *htim_echo;
    uint32_t echo_channel;
    TIM_HandleTypeDef *htim_trig;
    uint32_t trig_channel;
    uint16_t t_rise;
    uint16_t t_fall;
} ultrasonic_Handle_t;

/* Low-level ultrasonic functions */
void              Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor);
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t* ultrasonic_sensor);
uint8_t           Ultrasonic_IsReady(ultrasonic_Handle_t* ultrasonic_sensor);
float             Ultrasonic_GetDistance(ultrasonic_Handle_t* ultrasonic_sensor);

/* HAL will call this from the TIM input capture ISR (implemented in .c) */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

/* ========================================================================== */
/*         3) High-Level Rover Controller (Stepper + Ultrasonic FSM)         */
/* ========================================================================== */

/* Combined controller API used by main.c */
HAL_StatusTypeDef StepperUltrasonic_Init(ultrasonic_Handle_t *rightSensor,
                                         ultrasonic_Handle_t *leftSensor,
                                         Stepper28BYJ_Motor leftMotor,
                                         Stepper28BYJ_Motor rightMotor,
                                         float thresholdCm,
                                         uint32_t triggerIntervalMs);

void StepperUltrasonic_Process(void);

/* Called from HAL_TIM_PeriodElapsedCallback in main.c to service steppers */
void StepperUltrasonic_HandleStepperTimer(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* STEPPER_ULTRASONICSENSOR_H */
