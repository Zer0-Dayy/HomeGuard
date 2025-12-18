#include "system_manager.h"

static sensors_readings_t readings;
volatile system_state_t system_state = SYSTEM_SLEEP;
static system_phase_t system_phase = MOVEMENT_PHASE;

void System_Init(void)
{
	RTC_Service_Init();
    Comms_Init();
    Sensors_Init();
    Movement_Init();
}

void System_RunOnce(void)
{
	static uint32_t phase_start_tick = 0;

	if (system_state == SYSTEM_SLEEP)
	{
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
		SystemClock_Config();
		Movement_ReInitAfterStop();

		system_phase = MOVEMENT_PHASE;
		phase_start_tick = HAL_GetTick();
		return;
	}
	switch (system_phase)
	{
		case MOVEMENT_PHASE:
			Movement_Update();

			if((HAL_GetTick() - phase_start_tick) >= MOVEMENT_CYCLE)
			{
				system_phase = DETECTION_PHASE;
			}
			break;

		case DETECTION_PHASE:
			RTC_GetTimestamp(readings.timestamp, sizeof(readings.timestamp));
			Sensors_Read(&readings);
			Comms_Upload(&readings);
			HAL_Delay(5000);
			system_phase = MOVEMENT_PHASE;
			phase_start_tick = HAL_GetTick();
			break;

		default:
			system_phase = MOVEMENT_PHASE;
			break;
	}

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        system_state = (system_state == SYSTEM_RUNNING) ? SYSTEM_SLEEP : SYSTEM_RUNNING;
        if(system_state == SYSTEM_SLEEP)
        {
        	printf("LOW POWER MODE!\r\n");
        }
        else
        {
        	printf("SYSTEMS ONLINE!\r\n");
        }
    }
}

