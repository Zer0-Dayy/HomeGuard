#include "movement_service.h"


static ultrasonic_Handle_t sensorRight;
static ultrasonic_Handle_t sensorLeft;

static void check_status(HAL_StatusTypeDef st)
{
  if (!(st == HAL_OK || st == HAL_BUSY))
  {
    printf("Init error (%d)\r\n", st);
    Error_Handler();
  }
}


void Movement_Init(){
	sensorRight.htim_echo    = &htim3;

	sensorRight.echo_channel = TIM_CHANNEL_1;

	sensorRight.htim_trig    = &htim4;

	sensorRight.trig_channel = TIM_CHANNEL_2;


	sensorLeft.htim_echo    = &htim3;

	sensorLeft.echo_channel = TIM_CHANNEL_3;

	sensorLeft.htim_trig    = &htim4;



	sensorLeft.trig_channel = TIM_CHANNEL_3;

	check_status(Stepper28BYJ_Init(STEPPER28BYJ_MOTOR_A, &htim5,
	                                   GPIOB, GPIO_PIN_12,
	                                   GPIOB, GPIO_PIN_1,
	                                   GPIOB, GPIO_PIN_2,
	                                   GPIOB, GPIO_PIN_10));

	check_status(Stepper28BYJ_Init(STEPPER28BYJ_MOTOR_B, &htim5,
	                                   GPIOB, GPIO_PIN_3,
	                                   GPIOB, GPIO_PIN_4,
	                                   GPIOB, GPIO_PIN_5,
									   GPIOB, GPIO_PIN_11));

	check_status(Stepper28BYJ_SetSpeedPreset(STEPPER28BYJ_SPEED_MEDIUM));

	check_status(StepperUltrasonic_Init(&sensorRight, &sensorLeft,
	                                        STEPPER28BYJ_MOTOR_A, STEPPER28BYJ_MOTOR_B,
	                                        20.0f, 60U));

	printf("Running...\r\n");
}

void Movement_Update(void)
{
    StepperUltrasonic_Process();

}

void Movement_ReInitAfterStop(void)
{
    /* Timers are stopped in STOP mode */
    HAL_TIM_Base_Start_IT(&htim5);

    /* FSM already keeps state, just resume stepping */
}

void Movement_HandleTimerIRQ(TIM_HandleTypeDef *htim)
{
    StepperUltrasonic_HandleStepperTimer(htim);
}
