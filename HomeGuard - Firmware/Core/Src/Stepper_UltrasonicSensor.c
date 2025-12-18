#include "Stepper_UltrasonicSensor.h"
#include <stdio.h>

/* ========================================================================== */
/*                         1) Stepper28BYJ Driver                             */
/* ========================================================================== */

/* Internal state for each 28BYJ motor */
typedef struct
{
    TIM_HandleTypeDef *timer;
    GPIO_TypeDef *port[4];
    uint16_t pin[4];
    uint8_t stepIndex;
    int8_t direction;
    uint16_t stepsRemaining;
} Stepper28BYJ_Context;

/* Two motors: A and B share one timer */
static Stepper28BYJ_Context stepperCtx[STEPPER28BYJ_MOTOR_MAX] = {0};
static TIM_HandleTypeDef *sharedTimer = NULL;

/* 8‑step half‑step pattern for 28BYJ-48 */
static const uint8_t halfStepSequence[8] = {
    0b0001U, 0b0011U, 0b0010U, 0b0110U,
    0b0100U, 0b1100U, 0b1000U, 0b1001U
};

/* Drive the 4 GPIO pins for one motor with a 4‑bit pattern */
static void Stepper28BYJ_WriteOutputs(const Stepper28BYJ_Context *ctx, uint8_t pattern)
{
    HAL_GPIO_WritePin(ctx->port[0], ctx->pin[0], (pattern & 0b0001U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[1], ctx->pin[1], (pattern & 0b0010U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[2], ctx->pin[2], (pattern & 0b0100U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[3], ctx->pin[3], (pattern & 0b1000U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Return 1 if any motor still has steps to execute */
static uint8_t Stepper28BYJ_AnyBusy(void)
{
    for (uint8_t i = 0; i < STEPPER28BYJ_MOTOR_MAX; i++)
    {
        if (stepperCtx[i].stepsRemaining > 0U)
        {
            return 1U;
        }
    }
    return 0U;
}

/* Register one motor: store its pins/timer, reset state; ensure both share same timer */
HAL_StatusTypeDef Stepper28BYJ_Init(Stepper28BYJ_Motor motor,
                                    TIM_HandleTypeDef *timer,
                                    GPIO_TypeDef *portCoil1, uint16_t pinCoil1,
                                    GPIO_TypeDef *portCoil2, uint16_t pinCoil2,
                                    GPIO_TypeDef *portCoil3, uint16_t pinCoil3,
                                    GPIO_TypeDef *portCoil4, uint16_t pinCoil4)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX || timer == NULL)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    ctx->timer = timer;
    ctx->port[0] = portCoil1;
    ctx->pin[0] = pinCoil1;
    ctx->port[1] = portCoil2;
    ctx->pin[1] = pinCoil2;
    ctx->port[2] = portCoil3;
    ctx->pin[2] = pinCoil3;
    ctx->port[3] = portCoil4;
    ctx->pin[3] = pinCoil4;
    ctx->stepIndex = 0U;
    ctx->direction = STEPPER28BYJ_DIR_FORWARD;
    ctx->stepsRemaining = 0U;
    Stepper28BYJ_WriteOutputs(ctx, 0U);

    if (sharedTimer == NULL)
    {
        sharedTimer = timer;
    }
    else if (sharedTimer->Instance != timer->Instance)
    {
        return HAL_ERROR; /* both motors must share the same timer */
    }

    return HAL_OK;
}

/* Schedule a move for a motor and start timer interrupts if needed */
HAL_StatusTypeDef Stepper28BYJ_Move(Stepper28BYJ_Motor motor, uint16_t steps, int8_t direction)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX || sharedTimer == NULL || steps == 0U)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    if (ctx->timer == NULL)
    {
        return HAL_ERROR;
    }

    ctx->direction = (direction >= 0) ? STEPPER28BYJ_DIR_FORWARD : STEPPER28BYJ_DIR_REVERSE;
    ctx->stepsRemaining = steps;

    return HAL_TIM_Base_Start_IT(sharedTimer);
}

/* Stop one motor, de‑energize coils, and stop timer if no motors active */
HAL_StatusTypeDef Stepper28BYJ_Stop(Stepper28BYJ_Motor motor)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    if (ctx->timer == NULL)
    {
        return HAL_ERROR;
    }

    ctx->stepsRemaining = 0U;
    Stepper28BYJ_WriteOutputs(ctx, 0U);

    if (sharedTimer != NULL && Stepper28BYJ_AnyBusy() == 0U)
    {
        HAL_TIM_Base_Stop_IT(sharedTimer);
    }

    return HAL_OK;
}

/* Check if a motor still has steps pending */
uint8_t Stepper28BYJ_IsBusy(Stepper28BYJ_Motor motor)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX)
    {
        return 0U;
    }
    return (stepperCtx[motor].stepsRemaining > 0U) ? 1U : 0U;
}

/* Change overall step speed (timer prescaler) when no moves are active */
HAL_StatusTypeDef Stepper28BYJ_SetSpeedPreset(Stepper28BYJ_Speed preset)
{
    if (sharedTimer == NULL)
    {
        return HAL_ERROR;
    }

    if (Stepper28BYJ_AnyBusy() == 1U)
    {
        return HAL_BUSY;
    }

    uint32_t prescaler;
    switch (preset)
    {
        case STEPPER28BYJ_SPEED_LOW:
            prescaler = 16799U;
            break;
        case STEPPER28BYJ_SPEED_HIGH:
            prescaler = 4199U;
            break;
        case STEPPER28BYJ_SPEED_MEDIUM:
        default:
            prescaler = 8399U;
            break;
    }

    sharedTimer->Init.Prescaler = prescaler;
    sharedTimer->Init.Period = 9U;
    return HAL_TIM_Base_Init(sharedTimer);
}

/* Timer ISR hook: on each overflow, step any active motors; stop timer when all done */
void Stepper28BYJ_HandleTimerInterrupt(TIM_HandleTypeDef *htim)
{
    if ((sharedTimer == NULL) || (htim->Instance != sharedTimer->Instance))
    {
        return;
    }

    uint8_t anyActive = 0U;

    for (uint8_t motor = 0; motor < STEPPER28BYJ_MOTOR_MAX; motor++)
    {
        Stepper28BYJ_Context *ctx = &stepperCtx[motor];
        if ((ctx->timer == NULL) || (ctx->stepsRemaining == 0U))
        {
            continue;
        }

        anyActive = 1U;

        int8_t newIndex = (int8_t)ctx->stepIndex + ctx->direction;
        if (newIndex < 0)
        {
            newIndex = 7;
        }
        else if (newIndex > 7)
        {
            newIndex = 0;
        }
        ctx->stepIndex = (uint8_t)newIndex;

        Stepper28BYJ_WriteOutputs(ctx, halfStepSequence[ctx->stepIndex]);
        ctx->stepsRemaining--;
    }

    if (anyActive == 0U)
    {
        HAL_TIM_Base_Stop_IT(sharedTimer);
        for (uint8_t motor = 0; motor < STEPPER28BYJ_MOTOR_MAX; motor++)
        {
            if (stepperCtx[motor].timer != NULL)
            {
                Stepper28BYJ_WriteOutputs(&stepperCtx[motor], 0U);
            }
        }
    }
}

/* ========================================================================== */
/*                    2) Ultrasonic Sensor Driver (HC-SR04)                   */
/* ========================================================================== */

/* Simple registry of ultrasonic sensors using the same capture callback */
static ultrasonic_Handle_t* sensors[ULTRASONIC_MAX_SENSORS];
static uint8_t sensor_count = 0;

/* Register a sensor and start input‑capture interrupts on its echo channel */
void Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor)
{
    if(sensor_count < ULTRASONIC_MAX_SENSORS){
        sensors[sensor_count++] = ultrasonic_sensor;
    }
    ultrasonic_sensor->echo_state = WAITING_FOR_RISING;
    ultrasonic_sensor->ultrasonic_ready = 0;
    HAL_TIM_IC_Start_IT(ultrasonic_sensor->htim_echo, ultrasonic_sensor->echo_channel);
}

/* Emit a trigger pulse if the sensor is idle */
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t* ultrasonic_sensor){
    if(ultrasonic_sensor->ultrasonic_ready ||
       ultrasonic_sensor->echo_state == WAITING_FOR_FALLING){
        return ULTRASONIC_BUSY;
    }
    else{
        ultrasonic_sensor->ultrasonic_ready = 0;
        ultrasonic_sensor->t_fall = 0;
        ultrasonic_sensor->t_rise = 0;
        __HAL_TIM_DISABLE(ultrasonic_sensor->htim_trig);           // stop safe
        __HAL_TIM_SET_COUNTER(ultrasonic_sensor->htim_trig, 0);    // reset CNT
        HAL_TIM_PWM_Start(ultrasonic_sensor->htim_trig, ultrasonic_sensor->trig_channel);
        __HAL_TIM_ENABLE(ultrasonic_sensor->htim_trig);
    }

    return ULTRASONIC_OK;
}

/* HAL callback: handle echo rising/falling edges for all registered sensors */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < sensor_count; i++)
    {
        ultrasonic_Handle_t* s = sensors[i];

        /* Check if this interrupt is for this sensor’s echo timer */
        if (htim == s->htim_echo)
        {
            uint32_t active_channel = htim->Channel;

            if ((active_channel == HAL_TIM_ACTIVE_CHANNEL_1 && s->echo_channel == TIM_CHANNEL_1) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_2 && s->echo_channel == TIM_CHANNEL_2) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_3 && s->echo_channel == TIM_CHANNEL_3) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_4 && s->echo_channel == TIM_CHANNEL_4))
            {
                /* RISING edge: store t_rise, re‑arm for FALLING */
                if (s->echo_state == WAITING_FOR_RISING)
                {
                    s->t_rise = __HAL_TIM_GET_COMPARE(s->htim_echo, s->echo_channel);

                    TIM_IC_InitTypeDef cfg = {0};
                    cfg.ICPolarity  = TIM_INPUTCHANNELPOLARITY_FALLING;
                    cfg.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    cfg.ICFilter    = 0;
                    cfg.ICPrescaler = TIM_ICPSC_DIV1;
                    HAL_TIM_IC_ConfigChannel(s->htim_echo, &cfg, s->echo_channel);
                    HAL_TIM_IC_Start_IT(s->htim_echo, s->echo_channel);

                    s->echo_state = WAITING_FOR_FALLING;
                }
                /* FALLING edge: store t_fall, compute “ready”, re‑arm for RISING */
                else if (s->echo_state == WAITING_FOR_FALLING)
                {
                    s->t_fall = __HAL_TIM_GET_COMPARE(s->htim_echo, s->echo_channel);

                    TIM_IC_InitTypeDef cfg = {0};
                    cfg.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
                    cfg.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    cfg.ICFilter    = 0;
                    cfg.ICPrescaler = TIM_ICPSC_DIV1;
                    HAL_TIM_IC_ConfigChannel(s->htim_echo, &cfg, s->echo_channel);
                    HAL_TIM_IC_Start_IT(s->htim_echo, s->echo_channel);

                    s->ultrasonic_ready = 1;
                    s->echo_state = WAITING_FOR_RISING;
                }
            }
        }
    }
}

/* True if the sensor has a completed measurement ready to read */
uint8_t Ultrasonic_IsReady(ultrasonic_Handle_t* ultrasonic_sensor){
    return ultrasonic_sensor->ultrasonic_ready;
}

/* Convert echo time to distance in cm; returns NaN if not ready */
float Ultrasonic_GetDistance(ultrasonic_Handle_t* ultrasonic_sensor){
    if(Ultrasonic_IsReady(ultrasonic_sensor)==0){
        return NAN;
    }
    else{
        uint16_t time_difference = ultrasonic_sensor->t_fall - ultrasonic_sensor->t_rise ;
        ultrasonic_sensor->ultrasonic_ready = 0;
        float distance = (time_difference / 2.0f) * 0.0343f;
        return distance;
    }
}

/* ========================================================================== */
/*         3) High-Level Rover Controller (Stepper + Ultrasonic FSM)         */
/* ========================================================================== */

#define ROVER_STEPS_STRAIGHT   (512U)
#define ROVER_STEPS_TURN_FAST  (512U)
#define ROVER_STEPS_TURN_SLOW  (256U)

typedef enum
{
    ROUTE_MEASURE_RIGHT = 0U,
    ROUTE_MEASURE_LEFT,
    ROUTE_EVALUATE
} StepperUltrasonic_State_t;

typedef enum
{
    ROVER_CMD_STRAIGHT = 0U,
    ROVER_CMD_TURN_LEFT,
    ROVER_CMD_TURN_RIGHT
} StepperUltrasonic_Command_t;

/* Local handles to the two ultrasonic sensors and motors */
static ultrasonic_Handle_t *sensorRight = NULL;
static ultrasonic_Handle_t *sensorLeft  = NULL;
static Stepper28BYJ_Motor motorLeft  = STEPPER28BYJ_MOTOR_A;
static Stepper28BYJ_Motor motorRight = STEPPER28BYJ_MOTOR_B;

static float    obstacleThresholdCm = 20.0f;
static uint32_t triggerIntervalMs   = 60U;

static StepperUltrasonic_State_t  fsmState          = ROUTE_MEASURE_RIGHT;
static StepperUltrasonic_Command_t desiredCommand   = ROVER_CMD_STRAIGHT;
static StepperUltrasonic_Command_t announcedCommand = ROVER_CMD_STRAIGHT;

static uint32_t lastTriggerTick  = 0U;
static uint32_t lastAnnounceTick = 0U;
static float distanceRight = 0.0f;
static float distanceLeft  = 0.0f;
static uint8_t controllerReady = 0U;

/* --------- Internal helpers ------------------------------------------------ */
static void StepperUltrasonic_IssueMotion(StepperUltrasonic_Command_t cmd);
static void StepperUltrasonic_PrintCommand(StepperUltrasonic_Command_t cmd);

/* Initialize combined controller: link sensors/motors and start first move */
HAL_StatusTypeDef StepperUltrasonic_Init(ultrasonic_Handle_t *rightSensor,
                                         ultrasonic_Handle_t *leftSensor,
                                         Stepper28BYJ_Motor leftMotor,
                                         Stepper28BYJ_Motor rightMotor,
                                         float thresholdCm,
                                         uint32_t triggerIntervalMsParam)
{
    if ((rightSensor == NULL) || (leftSensor == NULL))
    {
        return HAL_ERROR;
    }

    sensorRight = rightSensor;
    sensorLeft  = leftSensor;
    motorLeft   = leftMotor;
    motorRight  = rightMotor;

    obstacleThresholdCm = thresholdCm;
    triggerIntervalMs   = (triggerIntervalMsParam == 0U) ? 60U : triggerIntervalMsParam;

    Ultrasonic_Sensor_Init(sensorRight);
    Ultrasonic_Sensor_Init(sensorLeft);

    fsmState = ROUTE_MEASURE_RIGHT;
    desiredCommand = ROVER_CMD_STRAIGHT;
    announcedCommand = ROVER_CMD_STRAIGHT;
    lastTriggerTick = HAL_GetTick();
    lastAnnounceTick = lastTriggerTick;
    distanceRight = 0.0f;
    distanceLeft  = 0.0f;

    controllerReady = 1U;

    StepperUltrasonic_IssueMotion(desiredCommand);
    StepperUltrasonic_PrintCommand(desiredCommand);

    return HAL_OK;
}

/* Main FSM: trigger sensors, read distances, decide route, queue next move */
void StepperUltrasonic_Process(void)
{
    if (controllerReady == 0U)
    {
        return;
    }

    switch (fsmState)
    {
        case ROUTE_MEASURE_RIGHT:
        {
            uint32_t now = HAL_GetTick();
            if ((now - lastTriggerTick) >= triggerIntervalMs)
            {
                if (Ultrasonic_Trigger(sensorRight) == ULTRASONIC_OK)
                {
                    lastTriggerTick = now;
                }
            }

            if (Ultrasonic_IsReady(sensorRight))
            {
                distanceRight = Ultrasonic_GetDistance(sensorRight);
                fsmState = ROUTE_MEASURE_LEFT;
            }
            break;
        }

        case ROUTE_MEASURE_LEFT:
        {
            uint32_t now = HAL_GetTick();
            if ((now - lastTriggerTick) >= triggerIntervalMs)
            {
                if (Ultrasonic_Trigger(sensorLeft) == ULTRASONIC_OK)
                {
                    lastTriggerTick = now;
                }
            }

            if (Ultrasonic_IsReady(sensorLeft))
            {
                distanceLeft = Ultrasonic_GetDistance(sensorLeft);
                fsmState = ROUTE_EVALUATE;
            }
            break;
        }

        case ROUTE_EVALUATE:
        {
            StepperUltrasonic_Command_t newCommand;

            if ((distanceRight >= obstacleThresholdCm) && (distanceLeft >= obstacleThresholdCm))
            {
                newCommand = ROVER_CMD_STRAIGHT;
            }
            else if (distanceRight >= distanceLeft)
            {
                newCommand = ROVER_CMD_TURN_RIGHT;
            }
            else
            {
                newCommand = ROVER_CMD_TURN_LEFT;
            }

            desiredCommand = newCommand;
            StepperUltrasonic_PrintCommand(newCommand);
            uint32_t now = HAL_GetTick();
            if ((announcedCommand != newCommand) || (now - lastAnnounceTick >= 1000U))
            {
                StepperUltrasonic_PrintCommand(newCommand);
                announcedCommand = newCommand;
                lastAnnounceTick = now;
            }

            fsmState = ROUTE_MEASURE_RIGHT;
            break;
        }

        default:
            fsmState = ROUTE_MEASURE_RIGHT;
            break;
    }

    if ((Stepper28BYJ_IsBusy(motorLeft) == 0U) &&
        (Stepper28BYJ_IsBusy(motorRight) == 0U))
    {
        StepperUltrasonic_IssueMotion(desiredCommand);
    }
}

/* Called from HAL_TIM_PeriodElapsedCallback in main.c to step the motors */
void StepperUltrasonic_HandleStepperTimer(TIM_HandleTypeDef *htim)
{
    Stepper28BYJ_HandleTimerInterrupt(htim);
}

/* Queue motion for both motors according to the chosen command */
static void StepperUltrasonic_IssueMotion(StepperUltrasonic_Command_t cmd)
{
    HAL_StatusTypeDef sL = HAL_ERROR;
    HAL_StatusTypeDef sR = HAL_ERROR;

    switch (cmd)
    {
        case ROVER_CMD_STRAIGHT:
            sL = Stepper28BYJ_Move(motorLeft,  ROVER_STEPS_STRAIGHT, STEPPER28BYJ_DIR_FORWARD);
            sR = Stepper28BYJ_Move(motorRight, ROVER_STEPS_STRAIGHT, STEPPER28BYJ_DIR_FORWARD);
            break;

        case ROVER_CMD_TURN_LEFT:
            sL = Stepper28BYJ_Move(motorLeft,  ROVER_STEPS_TURN_SLOW, STEPPER28BYJ_DIR_FORWARD);
            sR = Stepper28BYJ_Move(motorRight, ROVER_STEPS_TURN_FAST, STEPPER28BYJ_DIR_FORWARD);
            break;

        case ROVER_CMD_TURN_RIGHT:
            sL = Stepper28BYJ_Move(motorLeft,  ROVER_STEPS_TURN_FAST, STEPPER28BYJ_DIR_FORWARD);
            sR = Stepper28BYJ_Move(motorRight, ROVER_STEPS_TURN_SLOW, STEPPER28BYJ_DIR_FORWARD);
            break;

        default:
            return;
    }

    /* Ignore HAL_BUSY/ERROR on the second motor; timer is already running. */
    (void)sL;
    (void)sR;
}

/* Print navigation decision over UART */
static void StepperUltrasonic_PrintCommand(StepperUltrasonic_Command_t cmd)
{
    switch (cmd)
    {
        case ROVER_CMD_STRAIGHT:
            printf("Continue straight\r\n");
            break;
        case ROVER_CMD_TURN_LEFT:
            printf("Go left!\r\n");
            break;
        case ROVER_CMD_TURN_RIGHT:
            printf("Go right!\r\n");
            break;
        default:
            break;
    }
}
